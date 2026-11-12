#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Player { std::string name; float x,y,w,h; std::array<ImVec2, 10> joints; };
struct Frame { float time; std::vector<Player> players; };
std::vector<Frame> loadReplay(std::istream& input) {
    std::vector<Frame> frames;
    std::string line; int number=0;
    while (std::getline(input,line)) {
        ++number;
        if (line.find_first_not_of(" \t\r")==std::string::npos || line[line.find_first_not_of(" \t\r")]=='#') continue;
        std::istringstream row(line); float t; Player p; std::string extra;
        bool valid=bool(row>>t>>std::quoted(p.name)>>p.x>>p.y>>p.w>>p.h);
        for (auto& joint:p.joints) {
            valid=valid && bool(row>>joint.x>>joint.y);
            valid=valid && std::isfinite(joint.x) && std::isfinite(joint.y) && joint.x>=0 && joint.x<=1 && joint.y>=0 && joint.y<=1;
        }
        if (!valid || (row>>extra) ||
            !std::isfinite(t) || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.w) || !std::isfinite(p.h) ||
            t<0 || p.name.empty() || p.name.size()>128 || p.x<0 || p.y<0 || p.w<=0 || p.h<=0 || p.x+p.w>1 || p.y+p.h>1 ||
            (!frames.empty() && t<frames.back().time))
            throw std::runtime_error("Invalid replay row "+std::to_string(number));
        if (frames.empty() || frames.back().time!=t) frames.push_back({t,{}});
        auto& players=frames.back().players;
        if (std::any_of(players.begin(),players.end(),[&](const Player& v){return v.name==p.name;})) throw std::runtime_error("Duplicate player in frame");
        players.push_back(p);
    }
    if (frames.empty()) throw std::runtime_error("Replay has no frames");
    return frames;
}
struct Settings {
    bool names=true, boxes=true, skeleton=true;
    float thickness=2;
    ImVec4 nameColor{0.92f,0.95f,1,1}, boxColor{0.25f,0.8f,1,1}, boneColor{0.5f,1,0.65f,1};
};
void drawPlayers(ImDrawList* draw, ImVec2 origin, ImVec2 size, const Frame& frame, const Settings& s) {
    for (const auto& p:frame.players) {
        auto point=[&](float x,float y){return ImVec2(origin.x+(p.x+x*p.w)*size.x,origin.y+(p.y+y*p.h)*size.y);};
        if (s.boxes) draw->AddRect(point(0,0),point(1,1),ImGui::ColorConvertFloat4ToU32(s.boxColor),0,0,s.thickness);
        if (s.names) {
            auto pos=point(0.5f,0); pos.x-=ImGui::CalcTextSize(p.name.c_str()).x/2; pos.y-=20;
            draw->AddText(pos,ImGui::ColorConvertFloat4ToU32(s.nameColor),p.name.c_str());
        }
        // Joints are stored relative to the player bounds in the replay.
        if (s.skeleton) {
            const ImU32 color=ImGui::ColorConvertFloat4ToU32(s.boneColor);
            auto joint=[&](int i){return point(p.joints[i].x,p.joints[i].y);};
            draw->AddCircle(joint(0),p.w*size.x*.10f,color,16,s.thickness);
            const int bones[][2]={{0,1},{1,2},{2,3},{1,4},{4,5},{1,6},{6,7},{6,8},{8,9}};
            for (const auto& b:bones) draw->AddLine(joint(b[0]),joint(b[1]),color,s.thickness);
        }
    }
}
int selfTest() {
    const std::string pose=" .5 .1 .5 .3 .2 .3 .1 .5 .8 .3 .9 .5 .5 .6 .3 .95 .7 .8 .75 .95";
    std::istringstream good("0 \"A Player\" .1 .1 .2 .5"+pose+"\n1 B .2 .2 .2 .5"+pose+"\n");
    auto frames=loadReplay(good);
    if (frames.size()!=2 || frames[0].players[0].name!="A Player") return 1;
    const std::vector<std::string> bad={"", "0 A .9 .1 .2 .5"+pose,
        "1 A .1 .1 .2 .5"+pose+"\n0 B .1 .1 .2 .5"+pose,
        "0 A .1 .1 .2 .5"+pose+"\n0 A .1 .1 .2 .5"+pose,
        "0 A .1 .1 .2 .5", "0 A .1 .1 .2 .5"+pose+" extra"};
    for (const auto& text:bad) {
        try { std::istringstream input(text); loadReplay(input); return 1; } catch(const std::runtime_error&) {}
    }
    std::cout<<"Replay tests passed (valid, empty, bounds, ordering, duplicates, missing joints, extra fields)\n"; return 0;
}
int main(int argc,char** argv) {
    if (argc>1 && std::string(argv[1])=="--self-test") return selfTest();
    const bool smoke=argc>1 && std::string(argv[1])=="--smoke-test";
    const std::string path=argc>(smoke?2:1)?argv[smoke?2:1]:"data/sample.replay";
    try {
        std::ifstream input(path); if (!input) throw std::runtime_error("Cannot open "+path);
        const auto frames=loadReplay(input);
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)!=0) throw std::runtime_error(SDL_GetError());
        SDL_Window* window=SDL_CreateWindow("Replay Studio",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,1200,760,SDL_WINDOW_RESIZABLE|(smoke?SDL_WINDOW_HIDDEN:0));
        if (!window) throw std::runtime_error(SDL_GetError());
        SDL_Renderer* renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
        if (!renderer) throw std::runtime_error(SDL_GetError());
        IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::GetIO().IniFilename=nullptr; ImGui::StyleColorsDark();
        ImGui::GetStyle().WindowRounding=8; ImGui::GetStyle().FrameRounding=4;
        ImGui_ImplSDL2_InitForSDLRenderer(window,renderer); ImGui_ImplSDLRenderer2_Init(renderer);
        Settings settings; bool running=true, playing=true; float time=0; Uint64 previous=SDL_GetPerformanceCounter(); int rendered=0;
        while (running) {
            SDL_Event event; while (SDL_PollEvent(&event)) { ImGui_ImplSDL2_ProcessEvent(&event); if (event.type==SDL_QUIT) running=false; }
            Uint64 now=SDL_GetPerformanceCounter(); float dt=float(double(now-previous)/SDL_GetPerformanceFrequency()); previous=now;
            const float duration=frames.back().time;
            if (playing && duration>0) time=std::fmod(time+dt,duration);
            size_t index=0; while(index+1<frames.size() && frames[index+1].time<=time) ++index;
            ImGui_ImplSDLRenderer2_NewFrame(); ImGui_ImplSDL2_NewFrame(); ImGui::NewFrame();
            const ImVec2 display=ImGui::GetIO().DisplaySize;
            ImGui::SetNextWindowPos({0,0}); ImGui::SetNextWindowSize(display);
            ImGui::Begin("Replay Studio",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove);
            ImGui::TextUnformatted("REPLAY STUDIO / Player visualization"); ImGui::Separator();
            ImGui::BeginChild("Controls",{280,0},true);
            ImGui::TextUnformatted("DISPLAY");
            ImGui::Checkbox("Player names",&settings.names); ImGui::Checkbox("Boxes",&settings.boxes); ImGui::Checkbox("Skeleton",&settings.skeleton);
            ImGui::SliderFloat("Thickness",&settings.thickness,1,8,"%.1f px");
            ImGui::ColorEdit4("Names",&settings.nameColor.x); ImGui::ColorEdit4("Boxes",&settings.boxColor.x); ImGui::ColorEdit4("Bones",&settings.boneColor.x);
            if(ImGui::Button("Reset style")) settings=Settings{};
            ImGui::Separator(); ImGui::TextUnformatted("PLAYBACK");
            ImGui::Checkbox("Play",&playing); if(ImGui::Button("Restart")) time=0;
            ImGui::SliderFloat("Time",&time,0,duration,"%.2f s");
            ImGui::Text("Frame %zu / %zu",index+1,frames.size());
            ImGui::Text("Players: %zu",frames[index].players.size());
            ImGui::TextWrapped("Source: %s",path.c_str());
            ImGui::TextWrapped("Skeleton joints come from the replay.");
            ImGui::EndChild(); ImGui::SameLine();
            ImGui::BeginChild("Viewport",{0,0},true);
            ImVec2 origin=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();
            auto* draw=ImGui::GetWindowDrawList(); draw->AddRectFilled(origin,{origin.x+size.x,origin.y+size.y},IM_COL32(16,23,33,255));
            drawPlayers(draw,origin,size,frames[index],settings);
            ImGui::EndChild(); ImGui::End(); ImGui::Render();
            if (!ImGui::GetDrawData() || ImGui::GetDrawData()->TotalVtxCount==0) throw std::runtime_error("Empty UI render");
            SDL_SetRenderDrawColor(renderer,16,23,33,255); SDL_RenderClear(renderer); ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(),renderer); SDL_RenderPresent(renderer);
            if(smoke && ++rendered==3) running=false;
            SDL_Delay(16);
        }
        ImGui_ImplSDLRenderer2_Shutdown(); ImGui_ImplSDL2_Shutdown(); ImGui::DestroyContext(); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
        if(smoke) std::cout<<"Rendered 3 replay UI frames successfully\n";
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
