#include "FractalFrame.h"

//#include "menuicons.h"
#include "HDPrintscreenDialog.h"

///Constants
const mb::IterationT addIt = 100;
///Event enumeration
enum{
    ID_PRINTSCREEN    = 1,
    ID_HDPRINTSCREEN  = 2
};
///Constructor
FractalFrame::FractalFrame():wxFrame(nullptr, wxID_ANY, "Mandelbrot set plotter"){
    /**Menu*/{
        wxMenu* menuFile      = new wxMenu;
        wxMenuItem* menuItem_Printscreen   = new wxMenuItem(menuFile, ID_PRINTSCREEN  , wxT("Save printscreen\tCtrl+S"));
                    //menuItem_Printscreen  ->SetBitmap(FILESAVEAS);
        wxMenuItem* menuItem_HDPrintscreen = new wxMenuItem(menuFile, ID_HDPRINTSCREEN, wxT("Save HD printscreen"));
                    //menuItem_HDPrintscreen->SetBitmap(FILESAVEAS);
        menuFile->Append(menuItem_Printscreen  );
        menuFile->Append(menuItem_HDPrintscreen);

        wxMenuBar* menuBar = new wxMenuBar;
        menuBar->Append(menuFile, "File");
        this->SetMenuBar(menuBar);
    }
    /**Panels*/{
        fpanel = new FractalPanel(this, wxSize(1150, 500));
        ipanel = new InfoPanel   (this);
    }
    /**Background color*/{
        SetBackgroundColour(ipanel->GetBackgroundColour());
    }
    /**Sizers*/{
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(fpanel, 1, wxALL | wxEXPAND, 5);
        sizer->Add(ipanel, 0, wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND, 7);
        sizer->SetSizeHints(this);
        this->SetSizer(sizer);
    }
    /**Create fractal*/{
        f = new mb({-1.25L,0.0L}, 1.0L, fpanel->GetSize(), FractalHeight, true);
    }
    /**Create fractal thread*/{
        if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR){
            wxLogError("Could not create main thread");
            return;
        }
        if (GetThread()->Run() != wxTHREAD_NO_ERROR){
            wxLogError("Could not run main thread");
            return;
        }
        //fthread = new std::thread(Entry, this);
    }
}

typedef std::chrono::high_resolution_clock hrclock;
wxThread::ExitCode FractalFrame::Entry(){
    while(true){
        ///Update the fractal
        auto t1 = hrclock::now();
        f->UpdateMath(addIt);
        auto t2 = hrclock::now();
        ///Update the screen
        wxClientDC dc(fpanel);
        dc.DrawBitmap(*((wxBitmap*)f), 0, 0, true);
        ///Update the InfoPanel
        auto dt = std::chrono::duration<long double>(t2-t1);
        wxPoint p = wxGetMousePosition() - fpanel->GetScreenPosition();
        mb::ComplexNum c = f->GetOrigin() + mb::ComplexNum(+(mb::ComplexT)p.x*f->GetStep(),-(mb::ComplexT)p.y*f->GetStep());
        ipanel->Update(c, f->GetZoom(), f->GetNumIt(), dt.count()/(long double)addIt, f->GetHorizontalSize());
        ///Process events
        if(!fpanel->is_mouseevt_handled){ OnZoomEvent(fpanel->mouseevt); fpanel->is_mouseevt_handled = true; }
        if(!fpanel->is_sizeevt_handled ){ OnSizeEvent();                 fpanel->is_sizeevt_handled  = true; }
        if(!is_prtscevt_handled  ){ OnPrintscreenEvent();   is_prtscevt_handled   = true; }
        if(!is_hdprtscevt_handled){ CallAfter(OnHDPrintscreenEvent); is_hdprtscevt_handled = true; }
    }
    return (wxThread::ExitCode)0;
}

void FractalFrame::OnZoomEvent(const wxMouseEvent& evt){
    wxPoint p = wxGetMousePosition() - fpanel->GetScreenPosition();
    mb::ComplexNum newcenter = f->GetOrigin() + mb::ComplexNum(
        p.x*f->GetStep(), -p.y*f->GetStep()
    );
    mb::ComplexT newzoom = f->GetZoom()*std::pow(3.16227766017L, (long double)evt.GetWheelRotation()/evt.GetWheelDelta());
    delete f;
    f = new mb(newcenter, newzoom, fpanel->GetSize(), FractalHeight, true);
}

void FractalFrame::OnSizeEvent(){
    wxPoint p = wxGetMousePosition() - fpanel->GetScreenPosition();
    mb::ComplexNum newcenter = f->GetCenter();
    mb::ComplexT newzoom = f->GetZoom();
    delete f;
    f = new mb(newcenter, newzoom, fpanel->GetSize(), FractalHeight, true);
}


void NewImageName(const char* format, char* name){
    const unsigned long N = 100000;
    for(unsigned long i = 0; i < N; ++i){
        sprintf(name, format, i);
        if(!std::ifstream(name)) return;
    }
    throw std::logic_error("no available names");
}
void FractalFrame::OnPrintscreenEvent() const{
    char new_path[256];
    NewImageName(".\\Printscreens\\Image_%04d.png", new_path);
    if(f->SaveFile(new_path, wxBITMAP_TYPE_PNG))
        wxLogMessage("Printscreen saved as " + wxString(new_path));
}
void FractalFrame::OnHDPrintscreenEvent(){
    char new_path[256];
    NewImageName(".\\Printscreens\\Image_%04d.png", new_path);

    mb::ComplexNum center = f->GetCenter();
    mb::ComplexT zoom = f->GetZoom();
    wxSize sz = f->GetSize();
    mb::IterationT numIt = f->GetNumIt();
    HDPrintscreenDialog *dialog = new HDPrintscreenDialog(this, &center, &zoom, &sz, &numIt);
    if(dialog->ShowModal() != wxID_OK) return;
    mb *g = new mb(center, zoom, sz, FractalHeight, true);

    numIt = (numIt/addIt)*addIt + (numIt%addIt? addIt : 0);
    g->UpdateMath(numIt);
    if(g->SaveFile(new_path, wxBITMAP_TYPE_PNG))
        wxLogMessage("Printscreen saved as " + wxString(new_path));
}

///MACROS - redirect events to functions
wxBEGIN_EVENT_TABLE(FractalFrame, wxFrame)
    EVT_MENU(ID_PRINTSCREEN  , FractalFrame::OnPrintscreenCallback  )
    EVT_MENU(ID_HDPRINTSCREEN, FractalFrame::OnHDPrintscreenCallback)
wxEND_EVENT_TABLE()
