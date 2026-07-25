#include "Core/Engine.h"
#include "Core/GameLoop.h"
#include "Core/Renderer.h"
#include "Core/Input.h"
#include "Chart/ChartLoader.h"
#include "Chart/ChartTypes.h"
#include "Gameplay/Judge.h"
#include "Resource/ResourceManager.h"
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cmath>
#include <sstream>
#include <fstream>
#include <cstdlib>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace fs=std::filesystem;
enum class Page{MainMenu,Settings,ChartSelect,Play,Results,Quit};
static bool ttfOk=false;
static TTF_Font* ttfFont=nullptr;

static const char* KN(SDL_Scancode s){switch(s){case SDL_SCANCODE_A:return"A";case SDL_SCANCODE_B:return"B";case SDL_SCANCODE_C:return"C";case SDL_SCANCODE_D:return"D";case SDL_SCANCODE_E:return"E";case SDL_SCANCODE_F:return"F";case SDL_SCANCODE_G:return"G";case SDL_SCANCODE_H:return"H";case SDL_SCANCODE_I:return"I";case SDL_SCANCODE_J:return"J";case SDL_SCANCODE_K:return"K";case SDL_SCANCODE_L:return"L";case SDL_SCANCODE_M:return"M";case SDL_SCANCODE_N:return"N";case SDL_SCANCODE_O:return"O";case SDL_SCANCODE_P:return"P";case SDL_SCANCODE_Q:return"Q";case SDL_SCANCODE_R:return"R";case SDL_SCANCODE_S:return"S";case SDL_SCANCODE_T:return"T";case SDL_SCANCODE_U:return"U";case SDL_SCANCODE_V:return"V";case SDL_SCANCODE_W:return"W";case SDL_SCANCODE_X:return"X";case SDL_SCANCODE_Y:return"Y";case SDL_SCANCODE_Z:return"Z";default:return"?";}}
struct KB{SDL_Scancode k[8];int n=4;void D(){SDL_Scancode d[8]={SDL_SCANCODE_A,SDL_SCANCODE_S,SDL_SCANCODE_D,SDL_SCANCODE_F,SDL_SCANCODE_G,SDL_SCANCODE_H,SDL_SCANCODE_J,SDL_SCANCODE_K};for(int i=0;i<8;++i)k[i]=d[i];n=4;}int F(SDL_Scancode s)const{for(int i=0;i<n;++i)if(k[i]==s)return i;return-1;}};
static void AK(Ore::Input* io,const KB& kb){io->LoadDefaultBindings();for(int i=0;i<kb.n;++i)io->BindKey(kb.k[i],static_cast<Ore::GameAction>(static_cast<int>(Ore::GameAction::Lane0)+i));}
static void DTTF(Ore::Renderer* rdr,const std::string& t,int x,int y,uint8_t cr=255,uint8_t cg=255,uint8_t cb=255,uint8_t ca=255){if(ttfFont){SDL_Color c={cr,cg,cb,ca};SDL_Surface*s=TTF_RenderUTF8_Blended(ttfFont,t.c_str(),c);if(s){SDL_Texture* tx=SDL_CreateTextureFromSurface(rdr->GetSDLRenderer(),s);SDL_Rect dst={x,y,s->w,s->h};SDL_RenderCopy(rdr->GetSDLRenderer(),tx,nullptr,&dst);SDL_DestroyTexture(tx);SDL_FreeSurface(s);}}else rdr->DrawPixelText(t,x,y,cr,cg,cb,ca,2);}
static void DT(Ore::Renderer*r,const std::string& t,int x,int y,uint8_t cr=255,uint8_t cg=255,uint8_t cb=255,uint8_t ca=255){DTTF(r,t,x,y,cr,cg,cb,ca);}
static const char* JS(Ore::Judgment j){switch(j){case Ore::Judgment::Perfect:return"Perfect";case Ore::Judgment::Great:return"Great";case Ore::Judgment::Good:return"Good";case Ore::Judgment::Miss:return"Miss";default:return"";}}
static std::vector<std::string> SC(){std::vector<std::string> v;if(!fs::exists("Charts"))return v;for(auto& e:fs::directory_iterator("Charts")){auto p=e.path().extension().string();if(p==".mdc"||p==".json")v.push_back(e.path().filename().string());}std::sort(v.begin(),v.end());return v;}
static std::unique_ptr<Ore::Chart> ParseMdc(const std::string& path){std::ifstream f(path);if(!f.is_open())return nullptr;auto c=std::make_unique<Ore::Chart>();std::string ln;double bpm=120;int div=4;double step=60.0/bpm/div;double ct=0;bool first=true;int mL=0;auto AN=[&](double tm,int tr,int wt,Ore::NoteType ty,double dur=0){if(tr<1||tr>4)return;Ore::ChartNote n;n.timestamp=tm;n.lane=tr-1;n.type=ty;n.duration=dur;n.extras["weight"]=std::to_string(wt);c->notes.push_back(n);if(tr>mL)mL=tr;};while(std::getline(f,ln)){size_t s=ln.find_first_not_of(" \t\r\n");if(s==std::string::npos||ln[s]=='#')continue;ln=ln.substr(s);if(first&&ln[0]=='('&&ln.find("){")!=std::string::npos){size_t p1=ln.find(')'),p2=ln.find('{'),p3=ln.find('}');if(p1!=std::string::npos&&p2!=std::string::npos&&p3!=std::string::npos){bpm=std::stod(ln.substr(1,p1-1));div=std::stoi(ln.substr(p2+1,p3-p2-1));step=60.0/bpm/div;}first=false;continue;}if(ln[0]=='('&&ln.find("BPM=")!=std::string::npos){size_t eq=ln.find('='),cl=ln.find(')');if(eq!=std::string::npos&&cl!=std::string::npos)bpm=std::stod(ln.substr(eq+1,cl-eq-1));step=60.0/bpm/div;continue;}first=false;if(ln[0]=='@'){size_t cm=ln.find(',');std::string ats=ln.substr(1,cm-1);try{ct=std::stod(ats);}catch(...){}if(cm!=std::string::npos)ln=ln.substr(cm+1);else continue;}std::vector<std::string> toks;std::string cur;for(size_t i=0;i<=ln.size();++i){if(i==ln.size()||ln[i]==','){toks.push_back(cur);cur.clear();}else cur+=ln[i];}for(auto& tok:toks){if(tok.empty()){ct+=step;continue;}if(tok.find('/')!=std::string::npos){bool acc=tok.find('!')!=std::string::npos;std::string ts=tok;size_t pos=0;while(pos<ts.size()){size_t nxt=ts.find('/',pos);if(nxt==std::string::npos)nxt=ts.size();std::string sub=ts.substr(pos,nxt-pos);while(!sub.empty()&&(sub.back()=='!'||sub.back()=='h'))sub.pop_back();int tt=0;try{tt=std::stoi(sub);}catch(...){pos=nxt+1;continue;}if(tt>=1&&tt<=4)AN(ct,tt,acc?3:2,Ore::NoteType::Tap,0);pos=nxt+1;}ct+=step;continue;}int tr=tok[0]-'0';if(tr<1||tr>4){ct+=step;continue;}int sc=1;bool isH=false;double hd=0;if(tok.find('!')!=std::string::npos)sc=3;size_t hp=tok.find('h');if(hp!=std::string::npos){size_t lb=tok.find('[',hp),rb=tok.find(']',hp);if(lb!=std::string::npos&&rb!=std::string::npos){std::string ins=tok.substr(lb+1,rb-lb-1);size_t col=ins.find(':');double num=1,den=4;if(col!=std::string::npos){num=std::stod(ins.substr(0,col));den=std::stod(ins.substr(col+1));}hd=(num/den)*(60.0/bpm);isH=true;sc=2;}}AN(ct,tr,sc,isH?Ore::NoteType::Hold:Ore::NoteType::Tap,hd);ct+=step;}}c->metadata.title=fs::path(path).stem().string();c->metadata.artist="ORE";c->laneCount=mL>0?mL:4;c->bpmChanges.push_back({0.0,bpm});c->SortNotes();return c;}
static int NW(const Ore::ChartNote& n){auto it=n.extras.find("weight");return it!=n.extras.end()?std::stoi(it->second):1;}
struct JNote{double t;int l,w;Ore::NoteType ty;double d;bool j;};

int main(int,char**){
    std::cout<<"Open Rhythm Engine v0.2.0"<<std::endl;
    fs::path root=fs::current_path();
    {bool ok=false;std::string sp[]={".","..","../.."};for(auto& r:sp){if(fs::exists(root/r/"Charts")){root=fs::absolute(root/r);ok=true;break;}}if(!ok)root=fs::current_path();}
    fs::current_path(root);std::cout<<"Root: "<<root.string()<<std::endl;
    Ore::EngineConfig cfg;cfg.windowTitle="ORE v0.2.0";cfg.windowWidth=1280;cfg.windowHeight=720;
    Ore::Engine engine(cfg);if(!engine.Initialize()){std::cerr<<"Init fail"<<std::endl;return-1;}
    engine.GetResourceManager()->SetAssetRoot(root.string());
    auto*rdr=engine.GetRenderer();auto*inp=engine.GetInput();auto*cload=engine.GetChartLoader();
    if(TTF_Init()==0){const char*fs[]={"C:/Windows/Fonts/simhei.ttf","C:/Windows/Fonts/simsun.ttc","C:/Windows/Fonts/msyh.ttc","C:/Windows/Fonts/msyhbd.ttc","C:/Windows/Fonts/arial.ttf",nullptr};for(int i=0;fs[i];++i){ttfFont=TTF_OpenFont(fs[i],24);if(ttfFont){ttfOk=true;std::cout<<"TTF: "<<fs[i]<<std::endl;break;}}}
    if(!ttfOk)std::cout<<"Pixel fallback."<<std::endl;

    float freqs[8]={262.0f,294.0f,330.0f,349.0f,392.0f,440.0f,494.0f,523.0f};
    Mix_Chunk* beeps[8]={nullptr};Uint8 beepData[8][6000*2];
    for(int i=0;i<8;++i){auto* buf=(Sint16*)beepData[i];int ns=6000;for(int j=0;j<ns;++j){float t=(float)j/44100.0f;float env=1.0f-(float)j/ns;buf[j]=(Sint16)(sin(2.0*3.14159265*freqs[i]*t)*env*8000);}beeps[i]=Mix_QuickLoad_RAW(beepData[i],ns*2);}

    Page cur=Page::MainMenu,nx=Page::MainMenu;KB kb;kb.D();int ms=0;const int MI=3;int ss=0,SL=0,SK=1,SB=1;bool wk=false;int wL=-1;
    std::unique_ptr<Ore::Chart> chart;double lprs[8]={-10,-10,-10,-10,-10,-10,-10,-10},el=0,psT=0,tt=0;
    Ore::Judge judge;std::vector<JNote> jn;
    Ore::Judgment lj=Ore::Judgment::Miss;double jf=-10;
    double laneD[8]={0,0,0,0,0,0,0,0};double laneF[8]={-10,-10,-10,-10,-10,-10,-10,-10};

    struct SCORE{int p=0,gr=0,go=0,mi=0,tot=0,com=0,mx=0,ts=0;void R(){p=gr=go=mi=tot=com=mx=ts=0;}void H(Ore::Judgment j,int w){if(j==Ore::Judgment::Perfect){p++;com++;ts+=100*w;}else if(j==Ore::Judgment::Great){gr++;com++;ts+=80*w;}else if(j==Ore::Judgment::Good){go++;com++;ts+=60*w;}else{mi++;com=0;}if(com>mx)mx=com;}double A(){return tot==0?0:(p*1.0+gr*0.8+go*0.6)/tot*100;}}sc;
    std::vector<std::string> cfs=SC();int cs=0;bool rdy=false;AK(inp,kb);

    engine.GetGameLoop()->SetOnUpdate([&](double dt){tt+=dt;
        if(inp->IsActionPressed(Ore::GameAction::Pause)&&tt-psT>0.3){switch(cur){case Page::MainMenu:nx=Page::Quit;break;default:nx=Page::MainMenu;psT=tt;break;}}
        bool cf=inp->IsActionPressed(Ore::GameAction::Confirm)||inp->IsKeyPressed(SDL_SCANCODE_RETURN);bool pr=(tt-psT)>0.25;
        switch(cur){
        case Page::MainMenu:if(inp->IsKeyPressed(SDL_SCANCODE_UP))ms=(ms-1+MI)%MI;if(inp->IsKeyPressed(SDL_SCANCODE_DOWN))ms=(ms+1)%MI;if(cf&&pr){psT=tt;if(ms==0){cfs=SC();cs=0;nx=Page::ChartSelect;}else if(ms==1){ss=0;wk=false;nx=Page::Settings;}else nx=Page::Quit;}break;
        case Page::Settings:{if(wk){for(int k=SDL_SCANCODE_A;k<=SDL_SCANCODE_Z;++k)if(inp->IsKeyPressed((SDL_Scancode)k)){int oc=kb.F((SDL_Scancode)k);if(oc>=0&&oc!=wL)kb.k[oc]=kb.k[wL];kb.k[wL]=(SDL_Scancode)k;AK(inp,kb);wk=false;wL=-1;break;}if(inp->IsKeyPressed(SDL_SCANCODE_ESCAPE)){wk=false;wL=-1;}break;}int mx=1+kb.n+1;SB=mx-1;if(inp->IsKeyPressed(SDL_SCANCODE_UP))ss=(ss-1+mx)%mx;if(inp->IsKeyPressed(SDL_SCANCODE_DOWN))ss=(ss+1)%mx;if(ss==SL){if(inp->IsKeyPressed(SDL_SCANCODE_LEFT)){kb.n=std::max(2,kb.n-1);AK(inp,kb);}if(inp->IsKeyPressed(SDL_SCANCODE_RIGHT)){kb.n=std::min(8,kb.n+1);AK(inp,kb);}}if(cf&&pr){if(ss==SB){nx=Page::MainMenu;psT=tt;}else if(ss>=SK&&ss<SK+kb.n){wk=true;wL=ss-SK;}}if(inp->IsKeyPressed(SDL_SCANCODE_BACKSPACE)&&pr){nx=Page::MainMenu;psT=tt;}}break;
        case Page::ChartSelect:if(inp->IsKeyPressed(SDL_SCANCODE_UP))cs=std::max(0,cs-1);if(inp->IsKeyPressed(SDL_SCANCODE_DOWN))cs=std::min((int)cfs.size()-1,cs+1);if(cf&&pr&&!cfs.empty()){psT=tt;std::string cp="Charts/"+cfs[cs];chart=nullptr;if(cfs[cs].find(".mdc")!=std::string::npos)chart=ParseMdc(cp);else chart=cload->LoadChart(cp);if(chart){kb.n=chart->laneCount;AK(inp,kb);jn.clear();sc.R();for(auto&n:chart->notes){jn.push_back({n.timestamp,n.lane,NW(n),n.type,n.duration,false});sc.tot++;}el=0;rdy=false;jf=-10;lj=Ore::Judgment::Miss;for(int i=0;i<8;++i){laneF[i]=-10;laneD[i]=0;}nx=Page::Play;}else std::cout<<"[Err] "<<cp<<std::endl;}break;
        case Page::Play:if(rdy){if(cf&&pr){nx=Page::Results;psT=tt;}break;}el+=dt;{int lc=kb.n;for(int i=0;i<lc;++i){Ore::GameAction a=static_cast<Ore::GameAction>(static_cast<int>(Ore::GameAction::Lane0)+i);if(inp->IsActionPressed(a)){if(beeps[i])Mix_PlayChannel(-1,beeps[i],0);lprs[i]=el;double bd=999;int bi=-1;for(int ni=0;ni<(int)jn.size();++ni){if(jn[ni].j||jn[ni].l!=i)continue;double d2=std::abs(el-jn[ni].t);if(d2<bd&&d2<0.15){bd=d2;bi=ni;}}if(bi>=0){double delta=el-jn[bi].t;Ore::Judgment j=judge.JudgeHit(delta);jn[bi].j=true;sc.H(j,jn[bi].w);lj=j;jf=el;laneD[i]=delta;laneF[i]=el;}}}for(int ni=0;ni<(int)jn.size();++ni){if(!jn[ni].j&&el-jn[ni].t>0.15){jn[ni].j=true;sc.H(Ore::Judgment::Miss,jn[ni].w);lj=Ore::Judgment::Miss;jf=el;}}if(sc.p+sc.gr+sc.go+sc.mi>=sc.tot&&sc.tot>0)rdy=true;}break;
        case Page::Results:if(cf&&pr){nx=Page::ChartSelect;psT=tt;}break;default:break;}});

    engine.GetGameLoop()->SetOnRender([&](double){Ore::Renderer*r=rdr;int W=cfg.windowWidth,H=cfg.windowHeight,CX=W/2;
        if(nx!=cur){cur=nx;if(cur==Page::Play){el=0;for(int i=0;i<8;++i){laneF[i]=-10;laneD[i]=0;}}if(cur==Page::Quit)engine.Quit();}char b[256];
        switch(cur){
        case Page::MainMenu:r->ClearScreen(14,14,30);r->DrawRect(0,0,W,75,8,8,22);DT(r,"Open Rhythm Engine",20,10,255,220,80,255);DT(r,"v0.2.0",20,35,180,180,200,255);{const char*ls[3]={"开始游戏","设置","退出"};int cs[3][3]={{40,140,220},{80,80,180},{200,60,60}};for(int i=0;i<3;++i){int y=150+i*90,bx=CX-160;if(ms==i){r->DrawRect(bx-3,y-3,326,56,255,200,60,200);r->DrawRect(bx,y,320,50,cs[i][0]+40,cs[i][1]+30,cs[i][2]+20,255);}else r->DrawRect(bx,y,320,50,cs[i][0],cs[i][1],cs[i][2],200);DT(r,ls[i],CX-30,y+10,255,255,255,255);}}DT(r,"UP/DOWN:选择  ENTER:确认  ESC:退出",CX-150,600,160,160,200,255);break;
        case Page::Settings:r->ClearScreen(14,14,30);r->DrawRect(0,0,W,55,8,8,22);DT(r,"设置",20,10,255,200,60,255);{int y0=80,rh=38,mx=1+kb.n+1;SB=mx-1;{bool sl=(ss==SL);r->DrawRect(40,y0,320,rh,sl?50:25,sl?90:50,sl?150:80,200);if(sl)r->DrawRect(38,y0-2,324,rh+4,255,200,60,180);snprintf(b,sizeof(b),"轨道数: %dK  [<] [>]",kb.n);DT(r,b,50,y0+6,255,255,255,255);}y0+=rh+8;for(int i=0;i<kb.n;++i){bool sl=(ss==SK+i);r->DrawRect(40,y0,320,rh,sl?40:20,sl?40:25,sl?70:40,200);if(sl)r->DrawRect(38,y0-2,324,rh+4,255,200,60,180);snprintf(b,sizeof(b),wk&&wL==i?"轨道 %d: [按任意键...]":"轨道 %d: %s",i,KN(kb.k[i]));DT(r,b,50,y0+6,255,255,255,255);y0+=rh+4;}{bool sl=(ss==SB);r->DrawRect(40,y0+6,320,rh,sl?80:50,sl?40:30,sl?40:25,200);if(sl)r->DrawRect(38,y0+4,324,rh+4,255,200,60,180);DT(r,"[ 返回主菜单 ]",100,y0+12,255,200,180,255);}}r->DrawRect(420,80,W-440,400,18,18,36,180);DT(r,"操作说明:",430,90,255,220,100,255);DT(r,"UP/DOWN: 导航",430,120,200,200,220,255);DT(r,"LEFT/RIGHT: 切换轨道数",430,148,200,200,220,255);break;
        case Page::ChartSelect:r->ClearScreen(14,14,30);r->DrawRect(0,0,W,55,8,8,22);DT(r,"选择谱面",20,10,255,200,60,255);if(cfs.empty()){DT(r,"Charts/ 中没有谱面文件",CX-120,250,255,100,100,255);}else{int st=std::max(0,std::min(cs-5,(int)cfs.size()-11));for(int i=st;i<std::min((int)cfs.size(),st+11);++i){int y=80+(i-st)*50;bool sl=(i==cs);r->DrawRect(40,y,600,40,sl?50:20,sl?90:40,sl?150:70,200);if(sl)r->DrawRect(38,y-2,604,44,255,200,60,180);DT(r,cfs[i],50,y+6,255,255,255,255);}}DT(r,"ENTER:选择  ESC:返回",W-350,H-30,160,160,200,255);break;
        case Page::Play:r->ClearScreen(14,14,26);if(rdy){r->ClearScreen(14,14,28);DT(r,"谱面完成!",CX-60,200,255,255,100,255);snprintf(b,sizeof(b),"总分: %d",sc.ts);DT(r,b,CX-80,260,255,255,255,255);DT(r,"按 ENTER 查看详情",CX-80,320,200,200,220,255);break;}
            {int lc=kb.n,lw=90,lh=200,lsY=420;float sp=300;r->DrawRect(0,0,W,60,8,8,20);DT(r,"PLAY MODE",10,6,255,200,60,255);DT(r,"ESC返回",10,30,160,160,180,255);int sX=CX-(lc*lw)/2;for(int i=0;i<lc;++i){int x=sX+i*lw;r->DrawRect(x,lsY,lw-4,lh,30,30,48);r->DrawRect(x+lw-4,lsY,2,lh,45,45,68);int llw=60,llh=28,llx=x+(lw-4-llw)/2,ly=lsY+lh+8;r->DrawRect(llx,ly,llw,llh,40,40,65,180);DT(r,KN(kb.k[i]),llx+14,ly+4,255,255,255,255);
            if(el-laneF[i]<0.8){
                double d=laneD[i];const char* earlyLate="";uint8_t lr=255,lg=255,lb=255;
                if(d<-0.025){earlyLate="Early";lr=100;lg=180;lb=255;}else if(d>0.025){earlyLate="Late";lr=255;lg=80;lb=80;}else{earlyLate="OnTime";lr=100;lg=255;lb=100;}
                char lbuf[32];snprintf(lbuf,sizeof(lbuf),"%s (%.0fms)",earlyLate,d*1000);
                DT(r,lbuf,x+2,ly-18,lr,lg,lb,255);
            }
            }r->DrawRect(0,lsY-2,W,4,255,190,50,200);r->DrawRect(0,lsY+2,W,2,255,90,25,80);
            if(chart){for(auto&n:jn){if(n.j)continue;double dt=n.t-el;float ny=lsY-(float)(dt*sp);if(ny<-50||ny>lsY+600)continue;int nx=sX+n.l*lw+4,nw=lw-12,nh=16;if(n.ty==Ore::NoteType::Hold)nh=(int)(n.d*sp);if(nh<16)nh=16;int cR=220,cG=220,cB=60;if(n.w>=3){cR=255;cG=80;cB=80;}if(n.ty==Ore::NoteType::Hold){cG=140;cB=220;}r->DrawRect(nx,(int)ny,nw,nh,cR,cG,cB,220);}}
            {snprintf(b,sizeof(b),"进度: %d/%d",sc.p+sc.gr+sc.go+sc.mi,sc.tot);DT(r,b,W-240,55,200,200,220,255);snprintf(b,sizeof(b),"得分: %d",sc.ts);DT(r,b,W-240,75,220,220,100,255);}if(chart){snprintf(b,sizeof(b),"%s (%dK)",chart->metadata.title.c_str(),lc);DT(r,b,10,H-30,160,160,180,255);}}break;
        case Page::Results:r->ClearScreen(14,14,30);r->DrawRect(0,0,W,65,8,8,22);DT(r,"结算",20,12,255,200,60,255);{int x=80,y=100;snprintf(b,sizeof(b),"谱面: %s",chart?chart->metadata.title.c_str():"?");DT(r,b,x,y,255,255,255,255);y+=35;snprintf(b,sizeof(b),"Perfect: %d",sc.p);DT(r,b,x,y,255,220,60,255);y+=30;snprintf(b,sizeof(b),"Great:   %d",sc.gr);DT(r,b,x,y,100,255,100,255);y+=30;snprintf(b,sizeof(b),"Good:    %d",sc.go);DT(r,b,x,y,100,180,255,255);y+=30;snprintf(b,sizeof(b),"Miss:    %d",sc.mi);DT(r,b,x,y,255,80,80,255);y+=35;snprintf(b,sizeof(b),"总分: %d  连击: %d",sc.ts,sc.mx);DT(r,b,x,y,255,255,100,255);y+=35;double ac=sc.A();const char*gd="D";if(ac>=100)gd="SS";else if(ac>=95)gd="S";else if(ac>=90)gd="A";else if(ac>=80)gd="B";else if(ac>=65)gd="C";snprintf(b,sizeof(b),"精度: %.1f%%  评级: %s",ac,gd);DT(r,b,x,y,255,255,255,255);}DT(r,"按 ENTER 返回",CX-60,H-40,180,180,200,255);break;
        default:break;}});

    std::cout<<"\nStarted."<<std::endl;engine.Run();chart.reset();engine.Shutdown();
    if(ttfFont){TTF_CloseFont(ttfFont);ttfFont=nullptr;}TTF_Quit();
    for(int i=0;i<8;++i){if(beeps[i])Mix_FreeChunk(beeps[i]);}
    std::cout<<"Exit."<<std::endl;return 0;
}