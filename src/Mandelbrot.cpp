#include "Mandelbrot.h"


#include <fstream>
#include <iomanip>
#include <thread>

///Constructor
mb::mb(iter_t addIter):FractalBitmap(),px(*((wxBitmap*)this)),addIt(addIter){}

mb::mb(const mb &p):FractalBitmap(),px(*((wxBitmap*)this)),addIt(p.addIt){
    Create(p.GetCenter(), p.GetStep(), p.GetSize());
    std::lock_guard<std::mutex> lock(Mutex);
    numIt = p.numIt;
    const size_t N = GetWidth()*GetHeight();
    std::copy(p.C , p.C +N, C );
    std::copy(p.Z , p.Z +N, Z );
    std::copy(p.IT, p.IT+N, IT);
    std::copy(p.LCHK, p.LCHK+NThreads, LCHK);
}

///Create
void mb::Create(ComplexNum o, complex_t st, wxSize s){
    std::lock_guard<std::mutex> lock(Mutex);
    
    FractalBitmap::Create(o, st, s);

    px = wxNativePixelData(*((wxBitmap*)this));

    const unsigned N = GetWidth()*GetHeight();

    numIt = 0;
    if(C   !=NULL){ delete[] C ;   } C     = new ComplexNum[N]; //std::cout << sizeof(C) << std::endl;
    if(Z   !=NULL){ delete[] Z ;   } Z     = new ComplexNum[N]; std::fill(Z,Z+N,ComplexNum(complex_t(0.0L),complex_t(0.0L)));
    if(IT  !=NULL){ delete[] IT;   } IT    = new iter_t[N]; std::fill(IT,IT+N,0);
    if(LCHK!=NULL){ delete[] LCHK; } LCHK = new std::list<uint32_t>[NThreads];

    ///Fill 'C', 'LCHK' with new information
    uint32_t i = 0;
    ComplexNum c = GetOrigin();
    for(int y = 0; y < GetHeight(); ++y, c.imag(c.imag()-GetStep())){
        c.real(GetOrigin().real());
        for(int x = 0; x < GetWidth(); ++x, c.real(c.real()+GetStep()), ++i){
            C[i] = c;
            if(!isCardioid_isPeriod2Bulb(c)) LCHK[i*NThreads/N].push_back(i);
        }
    }
}

///Clone
mb* mb::Clone() const{
    return new mb(*this);
}

mb::~mb(){
    delete[] C;
    delete[] Z;
    delete[] IT;
    delete[] LCHK;
}

void mb::Update(){
    std::lock_guard<std::mutex> lock(Mutex);

    std::thread *ArrThreads[NThreads];
    std::deque<unsigned> vchanged[NThreads];
    for(unsigned i = 0; i < NThreads; ++i){
        ArrThreads[i] = new std::thread(&mb::UpdateMathLim, this, i, addIt, &(vchanged[i]));
    }
    for(unsigned i = 0; i < NThreads; ++i) ArrThreads[i]->join();
    for(unsigned i = 0; i < NThreads; ++i) delete ArrThreads[i];
    for(const auto& d:vchanged) UpdatePixels(d);

    numIt += addIt;

    BalanceLists();
}

void mb::UpdateMathLim(unsigned index, iter_t addIter, std::deque<unsigned>* changed){
    changed->clear();
    auto& L = LCHK[index];
    auto j = L.begin();
    bool ESCAPED;
    ComplexNum c;
    ComplexNum z;
    while(j != L.end()){
        ESCAPED = false;
        c = C[*j];

        z = Z[*j];
        iter_t nit;
        for(nit = addIter; --nit;){
            z*= z; z += c;
            if(std::norm(z) > bailout_sqr){
                Z[*j] = z; IT[*j] += addIter-nit;
                ESCAPED = true;
                changed->push_back(*j);
                j = L.erase(j);
                break;
            }
        }
        if(!ESCAPED){
            Z[*j] = z;
            IT[*j] += addIter-nit;
            ++j;





        }
    }

}

void mb::UpdatePixels(const std::deque<unsigned>& v){
    wxNativePixelData::Iterator p(px);
    
    for(const unsigned& i:v){
        p.MoveTo(px, i%GetWidth(), i/GetWidth());

        //x = (ColorT)IT[i]-3.0*(log2(0.5*log10(Z[i].absSqr()))-log2_log10N); ///continuous/wavy pattern
        //x = (ColorT)IT[i]-1.0L*(log2(0.5*log10(Z[i].absSqr()))-log2_log10N); ///continuous pattern, modified formula
        ColorT x = (ColorT)(IT[i]-(0.5L*log10((double)std::norm(Z[i]))/log10N-1.0L)); ///continuous pattern, original formula
        //x = (ColorT)IT[i]; ///discrete pattern

        ColorT y = CycleFun(omega*x + phi);
        p.Red()   = (unsigned char)(AMP[0]*y + INIT[0]);
        p.Green() = (unsigned char)(AMP[1]*y + INIT[1]);
        p.Blue()  = (unsigned char)(AMP[2]*y + INIT[2]);
    }
}

bool mb::SaveFile(const wxString& name, wxBitmapType type, const wxPalette *palette) const{
    std::lock_guard<std::mutex> lock(Mutex);

    wxBitmap::SaveFile(name+".png", type, palette);
    std::ofstream ostrm(name.ToStdString() + ".txt");

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Winline"
    ostrm << "timedate\t"    << wxDateTime::Now().Format("%d-%b-%Y %H:%M:%S").c_str() << "\n"
          << "timeelapsed\t" << std::setprecision( 8) << 0.0                          << "\n"
          << "re(c)\t"       << std::setprecision(20) << GetCenter().real()           << "\n"
          << "im(c)\t"       << std::setprecision(20) << GetCenter().imag()           << "\n"
          << "step\t"        << std::setprecision(20) << GetStep()                    << "\n"
          << "size.x\t"      << GetWidth()                                            << "\n"
          << "size.y\t"      << GetHeight()                                           << "\n"
          << "NumIt\t"       << numIt                                                 << "\n" << std::flush;
    #pragma GCC diagnostic pop

    ostrm.close();
    return true;
}

void mb::BalanceLists(){
    for(unsigned i = 0; i < NThreads-1; ++i){
        auto &L = LCHK[i], &R = LCHK[i+1];
        while(!R.empty() && L.size() < R.size()){
            L.push_back(R.front()); R.pop_front();
        }
        while(!L.empty() && L.size() > R.size()){
            R.push_front(L.back()); L.pop_back();
        }
    }
}

mb::ColorT mb::CycleFun(mb::ColorT x){
    x = remainderf(x, pi2);
    if(x < -pi_2) x = -pi-x;
    if(+pi_2 < x) x =  pi-x;
    return x/pi_2;                                ///Linear
}

mb::iter_t mb::GetNotEscaped() const{
    size_t ret = 0;
    for(unsigned i = 0; i < NThreads; ++i)
        ret += LCHK[i].size();
    return ret;
}
