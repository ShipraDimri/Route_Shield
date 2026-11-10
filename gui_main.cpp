// RouteShield - GUI frontend (C++17, Dear ImGui + GLFW + OpenGL3)
//
// Connected to the existing backend:
//   route.h / route.cpp          -> NetworkRouter::compute_optimal_path(...)
//   Intrusion.h / Intrusion.cpp  -> Intrusion (addTraffic, isBlacklisted, getBlacklist, ...)
//
// Put this file in the same folder as the four backend files and build all together.

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "Intrusion.h"
#include "route.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

const int kThreshold = 200;  // must match the limit given to Intrusion below

const ImU32 kGray  = IM_COL32(154, 154, 166, 255);
const ImU32 kGreen = IM_COL32(123, 227, 154, 255);
const ImU32 kRed   = IM_COL32(255, 107, 107, 255);
const ImU32 kAmber = IM_COL32(240, 160, 32, 255);

struct Link { int u, v; double latency; };
struct LogLine { std::string text; ImU32 color; };

float length(ImVec2 a, ImVec2 b) {
    return std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}

ImVec2 lerp2(ImVec2 a, ImVec2 b, float k) {
    return ImVec2(a.x + (b.x - a.x) * k, a.y + (b.y - a.y) * k);
}

// Moving white dashes along a line: this is the "shine" on the path.
void shineLine(ImDrawList* dl, ImVec2 a, ImVec2 b, double now) {
    float len = length(a, b);
    if (len < 1.0f) return;
    ImVec2 dir((b.x - a.x) / len, (b.y - a.y) / len);
    const float dash = 8.0f, gap = 22.0f, period = dash + gap;
    float offset = std::fmod((float)now * 55.0f, period);
    for (float s = -period + offset; s < len; s += period) {
        float s0 = std::max(s, 0.0f);
        float s1 = std::min(s + dash, len);
        if (s1 <= s0) continue;
        dl->AddLine(ImVec2(a.x + dir.x * s0, a.y + dir.y * s0),
                    ImVec2(a.x + dir.x * s1, a.y + dir.y * s1),
                    IM_COL32(255, 255, 255, 235), 2.5f);
    }
}

class App {
public:
    App() : intr(kThreshold) {
        // Normalized positions (0..1) so the graph scales with the window.
        pos = {{0.08f, 0.50f}, {0.28f, 0.17f}, {0.28f, 0.83f},
               {0.55f, 0.50f}, {0.62f, 0.88f}, {0.90f, 0.62f}};
        // Undirected links: router, router, latency.
        links = {{0, 1, 4}, {0, 2, 2}, {1, 3, 5}, {2, 3, 8},
                 {2, 4, 10}, {3, 4, 2}, {3, 5, 2}, {4, 5, 3}};
        traffic.assign(pos.size(), 0);
        src = 0;
        dst = 5;
        target = 1;
        logf(kGray, "RouteShield started: %d routers, %d links", (int)pos.size(), (int)links.size());
        recompute("initial route");
    }

    void frame(const ImGuiIO& io) {
        const double now = ImGui::GetTime();
        simulateAttack(io.DeltaTime, now);

        const float W = io.DisplaySize.x, H = io.DisplaySize.y;
        const float pad = 8.0f, cw = 330.0f;
        const float canvasH = (H - 3 * pad) * 0.70f;

        ImGui::SetNextWindowPos(ImVec2(pad, pad));
        ImGui::SetNextWindowSize(ImVec2(cw, H - 2 * pad));
        controlsWindow(now);

        ImGui::SetNextWindowPos(ImVec2(cw + 2 * pad, pad));
        ImGui::SetNextWindowSize(ImVec2(W - cw - 3 * pad, canvasH));
        canvasWindow(io, now);

        ImGui::SetNextWindowPos(ImVec2(cw + 2 * pad, 2 * pad + canvasH));
        ImGui::SetNextWindowSize(ImVec2(W - cw - 3 * pad, H - 3 * pad - canvasH));
        logWindow();
    }

private:
    // ---------- backend glue ----------
    std::vector<std::vector<std::pair<int, double>>> buildAdjacency() const {
        std::vector<std::vector<std::pair<int, double>>> adj(pos.size());
        for (const Link& l : links) {
            adj[l.u].push_back({l.v, l.latency});
            adj[l.v].push_back({l.u, l.latency});
        }
        return adj;
    }

    void recompute(const char* reason) {
        result = NetworkRouter::compute_optimal_path(
            (int)pos.size(), buildAdjacency(), src, dst, intr.getBlacklist());
        if (result.exists) {
            logf(kGreen, "[%s] path %s  (latency %.1f)", reason, pathString().c_str(), result.total_latency);
        } else if (intr.isBlacklisted(src) || intr.isBlacklisted(dst)) {
            logf(kRed, "[%s] source or destination is blacklisted", reason);
        } else {
            logf(kRed, "[%s] no safe path from R%d to R%d", reason, src, dst);
        }
    }

    void addTraffic(int id, int count, double now) {
        bool was = intr.isBlacklisted(id);
        traffic[id] += count;
        intr.addTraffic(id, count);  // backend decides if the threshold is crossed
        if (!was && intr.isBlacklisted(id)) {
            logf(kRed, "ALERT: R%d crossed %d requests and was blacklisted", id, kThreshold);
            flashUntil = now + 1.8;
            recompute("rerouting around blacklist");
        }
    }

    void restore(int id) {
        intr.removeFromBlacklist(id);
        intr.resetTraffic(id);
        traffic[id] = 0;
        logf(kAmber, "R%d restored and removed from blacklist", id);
        recompute("blacklist updated");
    }

    void resetAll() {
        intr = Intrusion(kThreshold);
        std::fill(traffic.begin(), traffic.end(), 0);
        attacking = false;
        logf(kGray, "All traffic counters and the blacklist were reset");
        recompute("reset");
    }

    void simulateAttack(float dt, double now) {
        if (!attacking) return;
        if (intr.isBlacklisted(target)) { attacking = false; return; }
        attackAccum += dt;
        while (attackAccum >= 0.12f) {
            attackAccum -= 0.12f;
            addTraffic(target, 8, now);
            if (intr.isBlacklisted(target)) { attacking = false; break; }
        }
    }

    // ---------- helpers ----------
    std::string pathString() const {
        std::string s;
        for (size_t i = 0; i < result.path.size(); i++) {
            if (i) s += " -> ";
            s += "R" + std::to_string(result.path[i]);
        }
        return s;
    }

    bool onPathEdge(int a, int b) const {
        if (!result.exists) return false;
        for (size_t i = 0; i + 1 < result.path.size(); i++) {
            int p = result.path[i], q = result.path[i + 1];
            if ((p == a && q == b) || (p == b && q == a)) return true;
        }
        return false;
    }

    void logf(ImU32 color, const char* fmt, ...) {
        char buf[256];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        logs.push_back({buf, color});
        if (logs.size() > 200) logs.erase(logs.begin());
        scrollLog = true;
    }

    bool routerCombo(const char* label, int& idx) {
        bool changed = false;
        char cur[16];
        std::snprintf(cur, sizeof cur, "R%d", idx);
        if (ImGui::BeginCombo(label, cur)) {
            for (int i = 0; i < (int)pos.size(); i++) {
                char item[16];
                std::snprintf(item, sizeof item, "R%d", i);
                if (ImGui::Selectable(item, i == idx)) {
                    if (idx != i) changed = true;
                    idx = i;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    // ---------- windows ----------
    void controlsWindow(double now) {
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("Controls", nullptr, flags);

        ImGui::TextColored(ImVec4(0.31f, 0.82f, 0.45f, 1.0f), "ROUTESHIELD");
        ImGui::TextDisabled("Adaptive routing + intrusion defense");
        ImGui::Separator();

        ImGui::TextUnformatted("Route request");
        bool changed = false;
        ImGui::SetNextItemWidth(110);
        changed |= routerCombo("Source", src);
        ImGui::SetNextItemWidth(110);
        changed |= routerCombo("Destination", dst);
        if (changed) recompute("route request changed");
        if (ImGui::Button("Find safest path", ImVec2(-1, 0))) recompute("manual request");

        if (result.exists) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.89f, 0.60f, 1.0f));
            ImGui::TextWrapped("%s", pathString().c_str());
            ImGui::Text("Total latency: %.1f", result.total_latency);
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.42f, 0.42f, 1.0f));
            ImGui::TextWrapped("No safe path available");
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Traffic simulator");
        ImGui::SetNextItemWidth(110);
        routerCombo("Target", target);
        if (ImGui::Button("+20")) addTraffic(target, 20, now);
        ImGui::SameLine();
        if (ImGui::Button("+100")) addTraffic(target, 100, now);
        ImGui::SameLine();
        if (ImGui::Button("Flood +250")) addTraffic(target, 250, now);
        if (ImGui::Checkbox("Simulate attack on target", &attacking) && attacking)
            logf(kAmber, "Attack simulation started on R%d", target);
        if (ImGui::Button("Reset target traffic", ImVec2(-1, 0))) {
            intr.resetTraffic(target);
            traffic[target] = 0;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Traffic per router");
        for (int i = 0; i < (int)pos.size(); i++) {
            bool bl = intr.isBlacklisted(i);
            float f = std::min(1.0f, (float)traffic[i] / (float)kThreshold);
            ImVec4 col = bl ? ImVec4(0.85f, 0.20f, 0.20f, 1.0f)
                       : f < 0.5f ? ImVec4(0.24f, 0.70f, 0.39f, 1.0f)
                       : f < 0.8f ? ImVec4(0.90f, 0.63f, 0.16f, 1.0f)
                                  : ImVec4(0.90f, 0.30f, 0.20f, 1.0f);
            char overlay[32];
            if (bl) std::snprintf(overlay, sizeof overlay, "BLACKLISTED");
            else std::snprintf(overlay, sizeof overlay, "%d / %d", traffic[i], kThreshold);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("R%d", i);
            ImGui::SameLine(44);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
            ImGui::ProgressBar(f, ImVec2(-1, 0), overlay);
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Blacklist");
        if (intr.getBlacklist().empty()) {
            ImGui::TextDisabled("No blacklisted routers");
        } else {
            for (int i = 0; i < (int)pos.size(); i++) {
                if (!intr.isBlacklisted(i)) continue;
                ImGui::PushID(i);
                ImGui::TextColored(ImVec4(1.0f, 0.42f, 0.42f, 1.0f), "R%d", i);
                ImGui::SameLine(80);
                if (ImGui::SmallButton("Restore")) restore(i);
                ImGui::PopID();
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Reset everything", ImVec2(-1, 0))) resetAll();
        ImGui::TextDisabled("Tip: drag routers with the mouse");
        ImGui::End();
    }

    void canvasWindow(const ImGuiIO& io, double now) {
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                       ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                       ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::Begin("Network Topology", nullptr, flags);

        const ImVec2 o = ImGui::GetCursorScreenPos();
        const ImVec2 sz = ImGui::GetContentRegionAvail();
        const ImVec2 end(o.x + sz.x, o.y + sz.y);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->PushClipRect(o, end, true);
        dl->AddRectFilled(o, end, IM_COL32(18, 19, 24, 255), 8.0f);

        auto px = [&](int i) { return ImVec2(o.x + pos[i].x * sz.x, o.y + pos[i].y * sz.y); };
        const std::unordered_set<int>& bl = intr.getBlacklist();
        const float R = 20.0f;
        const float pulse = 0.5f + 0.5f * std::sin((float)now * 3.0f);

        std::vector<char> onPath(pos.size(), 0);
        if (result.exists) for (int id : result.path) onPath[id] = 1;

        // base edges
        for (const Link& l : links) {
            bool dead = bl.count(l.u) || bl.count(l.v);
            dl->AddLine(px(l.u), px(l.v), dead ? IM_COL32(80, 55, 60, 255) : IM_COL32(120, 120, 132, 255),
                        dead ? 1.5f : 2.0f);
        }

        // glowing, shining path
        if (result.exists && result.path.size() > 1) {
            for (size_t i = 0; i + 1 < result.path.size(); i++) {
                ImVec2 a = px(result.path[i]), b = px(result.path[i + 1]);
                dl->AddLine(a, b, IM_COL32(40, 200, 100, (int)(25 + 25 * pulse)), 20.0f);
                dl->AddLine(a, b, IM_COL32(60, 230, 120, (int)(45 + 35 * pulse)), 12.0f);
                dl->AddLine(a, b, IM_COL32(52, 211, 111, 255), 5.0f);
                shineLine(dl, a, b, now);
            }
        }

        // latency labels
        for (const Link& l : links) {
            ImVec2 a = px(l.u), b = px(l.v);
            float len = length(a, b);
            if (len < 1.0f) continue;
            ImVec2 m((a.x + b.x) / 2 - (b.y - a.y) / len * 14, (a.y + b.y) / 2 + (b.x - a.x) / len * 14);
            char buf[16];
            std::snprintf(buf, sizeof buf, "%.0f", l.latency);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            dl->AddText(ImVec2(m.x - ts.x / 2, m.y - ts.y / 2), IM_COL32(255, 220, 90, 255), buf);
        }

        // routers
        for (int i = 0; i < (int)pos.size(); i++) {
            ImVec2 c = px(i);
            bool dead = bl.count(i) > 0;

            ImGui::SetCursorScreenPos(ImVec2(c.x - R, c.y - R));
            ImGui::PushID(i);
            ImGui::InvisibleButton("router", ImVec2(R * 2, R * 2));
            if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                pos[i].x = std::min(0.96f, std::max(0.04f, pos[i].x + io.MouseDelta.x / sz.x));
                pos[i].y = std::min(0.94f, std::max(0.08f, pos[i].y + io.MouseDelta.y / sz.y));
            }
            ImGui::PopID();

            if (dead) {
                dl->AddCircle(c, R + 6 + 5 * pulse, IM_COL32(255, 70, 70, (int)(90 + 120 * pulse)), 0, 3.0f);
            } else if (onPath[i]) {
                dl->AddCircle(c, R + 5 + 4 * pulse, IM_COL32(110, 245, 160, (int)(80 + 140 * pulse)), 0, 2.5f);
            }
            ImU32 fill = dead ? IM_COL32(210, 50, 50, 255)
                       : onPath[i] ? IM_COL32(47, 209, 106, 255)
                                   : IM_COL32(50, 110, 200, 255);
            dl->AddCircleFilled(c, R, fill);
            dl->AddCircle(c, R, IM_COL32(255, 255, 255, 255), 0, 2.0f);

            char name[16];
            std::snprintf(name, sizeof name, "R%d", i);
            ImVec2 ts = ImGui::CalcTextSize(name);
            dl->AddText(ImVec2(c.x - ts.x / 2, c.y - ts.y / 2), IM_COL32(255, 255, 255, 255), name);

            const char* tag = (i == src) ? "SOURCE" : (i == dst) ? "DESTINATION" : nullptr;
            if (tag) {
                ImVec2 tt = ImGui::CalcTextSize(tag);
                dl->AddText(ImVec2(c.x - tt.x / 2, c.y - R - 8 - tt.y), kGray, tag);
            }
            if (dead) {
                ImVec2 tt = ImGui::CalcTextSize("BLOCKED");
                dl->AddText(ImVec2(c.x - tt.x / 2, c.y + R + 6), kRed, "BLOCKED");
            }
        }

        // packet travelling along the safe path, with a soft halo
        if (result.exists && result.path.size() > 1) {
            std::vector<ImVec2> pts;
            for (int id : result.path) pts.push_back(px(id));
            float total = 0.0f;
            for (size_t i = 0; i + 1 < pts.size(); i++) total += length(pts[i], pts[i + 1]);
            if (total > 1.0f) {
                float d = std::fmod((float)now * 170.0f, total + 80.0f);
                if (d < total) {
                    float acc = 0.0f;
                    for (size_t i = 0; i + 1 < pts.size(); i++) {
                        float l = length(pts[i], pts[i + 1]);
                        if (d <= acc + l) {
                            ImVec2 p = lerp2(pts[i], pts[i + 1], (d - acc) / l);
                            dl->AddCircleFilled(p, 15.0f, IM_COL32(123, 227, 154, 70));
                            dl->AddCircleFilled(p, 9.0f, IM_COL32(123, 227, 154, 130));
                            dl->AddCircleFilled(p, 6.0f, IM_COL32(255, 255, 255, 255));
                            dl->AddCircle(p, 6.0f, IM_COL32(47, 209, 106, 255), 0, 2.5f);
                            break;
                        }
                        acc += l;
                    }
                }
            }
        }

        // status pill
        const char* status;
        ImU32 pillBg, pillFg;
        if (now < flashUntil) {
            status = "THREAT DETECTED"; pillBg = IM_COL32(74, 22, 22, 255); pillFg = IM_COL32(255, 138, 138, 255);
        } else if (!result.exists) {
            status = "NO SAFE PATH"; pillBg = IM_COL32(74, 22, 22, 255); pillFg = IM_COL32(255, 138, 138, 255);
        } else if (!bl.empty()) {
            status = "REROUTED - SAFE"; pillBg = IM_COL32(60, 48, 12, 255); pillFg = IM_COL32(255, 200, 90, 255);
        } else {
            status = "SECURE"; pillBg = IM_COL32(18, 57, 31, 255); pillFg = IM_COL32(123, 227, 154, 255);
        }
        ImVec2 st = ImGui::CalcTextSize(status);
        ImVec2 p0(end.x - st.x - 36, o.y + 12), p1(end.x - 12, o.y + 12 + st.y + 10);
        dl->AddRectFilled(p0, p1, pillBg, 14.0f);
        dl->AddText(ImVec2(p0.x + 12, p0.y + 5), pillFg, status);

        if (now < flashUntil) {
            float a = 0.5f + 0.5f * std::sin((float)now * 14.0f);
            dl->AddRect(o, end, IM_COL32(255, 60, 60, (int)(80 + 175 * a)), 8.0f, 0, 4.0f);
        }

        // legend
        const struct { ImU32 c; const char* t; } legend[] = {
            {IM_COL32(47, 209, 106, 255), "On safe path"},
            {IM_COL32(50, 110, 200, 255), "Normal"},
            {IM_COL32(210, 50, 50, 255), "Blacklisted"}};
        float lx = o.x + 14;
        float ly = end.y - 18;
        for (const auto& item : legend) {
            dl->AddCircleFilled(ImVec2(lx + 5, ly), 5.0f, item.c);
            dl->AddText(ImVec2(lx + 16, ly - 7), kGray, item.t);
            lx += 24 + ImGui::CalcTextSize(item.t).x + 14;
        }

        dl->PopClipRect();
        ImGui::End();
    }

    void logWindow() {
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
        ImGui::Begin("Event log", nullptr, flags);
        if (ImGui::SmallButton("Clear")) logs.clear();
        ImGui::Separator();
        ImGui::BeginChild("logscroll");
        for (const LogLine& l : logs) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(l.color));
            ImGui::TextUnformatted(l.text.c_str());
            ImGui::PopStyleColor();
        }
        if (scrollLog) {
            ImGui::SetScrollHereY(1.0f);
            scrollLog = false;
        }
        ImGui::EndChild();
        ImGui::End();
    }

    // ---------- state ----------
    std::vector<ImVec2> pos;
    std::vector<Link> links;
    std::vector<int> traffic;  // GUI-side mirror of request counts (for the bars)
    Intrusion intr;            // backend: traffic tracking + blacklist
    PacketPath result{};       // backend: latest computed path
    int src = 0, dst = 5, target = 1;
    bool attacking = false;
    float attackAccum = 0.0f;
    double flashUntil = -1.0;
    std::vector<LogLine> logs;
    bool scrollLog = false;
};

}  // namespace

int main() {
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 760, "RouteShield", nullptr, nullptr);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.ItemSpacing = ImVec2(8, 8);
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.11f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.29f, 0.48f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.24f, 0.44f, 0.72f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.52f, 0.82f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.38f, 0.64f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    App app;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.frame(io);

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
