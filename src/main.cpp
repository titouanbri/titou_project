#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif


std::string portCOM = "COM3"; 

void SendToArduino(const std::string& portName, const std::string& data) {
#ifdef _WIN32
    // Remarque : Pour les ports COM >= 10, le format doit être "\\\\.\\COM10"
    std::string realPortName = "\\\\.\\" + portName; 
    
    HANDLE hSerial = CreateFile(realPortName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error: Impossible to open serial port " << portName << std::endl;
        return;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error: impossible to get the serial port state" << std::endl;
        CloseHandle(hSerial);
        return;
    }

    // Configuration classique pour un Arduino (9600 bauds, 8N1)
    dcbSerialParams.BaudRate = CBR_9600; 
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error: Impossible to configure the serial port" << std::endl;
        CloseHandle(hSerial);
        return;
    }

    DWORD bytesWritten;
    if (!WriteFile(hSerial, data.c_str(), data.length(), &bytesWritten, NULL)) {
        std::cerr << "Error: Impossible to write to the serial port" << std::endl;
    } else {
        std::cout << "Success: '" << data << "' sent to the Arduino on " << portName << std::endl;
    }

    CloseHandle(hSerial);

#else
    // --- CODE LINUX (Simulation pour le développement) ---
    std::cout << "[LINUX DEBUG] Simulation d'envoi série sur " << portName << " : " << data << std::endl;
#endif
}

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}


bool ToggleButton(const char* str_id, bool* v) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    bool changed = false;

    // Créer un bouton invisible pour gérer le clic
    ImGui::InvisibleButton(str_id, ImVec2(width, height));
    if (ImGui::IsItemClicked())
    {
        *v = !*v;
        changed = true;
    }
    // Déterminer la couleur de fond
    ImU32 col_bg;
    if (ImGui::IsItemHovered())
        col_bg = ImGui::GetColorU32(*v ? ImGuiCol_ButtonActive : ImGuiCol_FrameBgHovered);
    else
        col_bg = ImGui::GetColorU32(*v ? ImGuiCol_Button : ImGuiCol_FrameBg);

    // Dessiner le fond (rect aux bords arrondis) et le cercle (le switch)
    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), col_bg, height * 0.5f);
    draw_list->AddCircleFilled(ImVec2(p.x + radius + (*v ? 1 : 0) * (width - radius * 2.0f), p.y + radius), radius - 1.5f, IM_COL32(255, 255, 255, 255));
    
    return changed;
}






int main() {
    // Initialisation de GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // Configuration d'OpenGL 3.0+
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Création de la fenêtre
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Titou_project", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Activer la V-Sync (pour pas saturer le gpu)

    // Initialisation de Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.5f;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Activer le clavier
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Activer le docking

    // Style de l'interface
    ImGui::StyleColorsDark();

    // Configuration des backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Variables pour notre interface
    bool show_demo_window = false;

    ImVec4 color1 = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Rouge
    ImVec4 color2 = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Vert
    ImVec4 color3 = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); // Bleu

    bool led_state = false;
    bool pot_state = false;
    bool silence_state = false;

    // Boucle principale
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Démarrer une nouvelle frame ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Afficher la fenêtre de démo d'ImGui (très utile pour apprendre)
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Créer votre propre fenêtre
        {
            // Récupérer le viewport principal (la fenêtre GLFW)
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            
            // Forcer la position et la taille de la prochaine fenêtre
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);

            // Définir les options (flags) pour bloquer la fenêtre et la rendre "invisible" en tant que conteneur
            ImGuiWindowFlags window_flags = 
                ImGuiWindowFlags_NoTitleBar |       // Pas de barre de titre (fermeture, etc.)
                ImGuiWindowFlags_NoCollapse |       // Impossible de la réduire
                ImGuiWindowFlags_NoResize |         // Impossible de la redimensionner
                ImGuiWindowFlags_NoMove |           // Impossible de la déplacer
                ImGuiWindowFlags_NoBringToFrontOnFocus | // Ne passe pas au premier plan quand on clique dessus
                ImGuiWindowFlags_NoNavFocus;        // Pas de focus clavier par défaut

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            ImGui::Begin("Ma Première Fenêtre",NULL, window_flags);
            ImGui::PopStyleVar();
            ImGui::Text("oui oui baguette");
            ImGui::Checkbox("Afficher la démo ImGui", &show_demo_window);
            
            ImGui::ColorEdit3("Couleur 1 ", (float*)&color1);
            ImGui::ColorEdit3("Couleur 2 ", (float*)&color2);
            ImGui::ColorEdit3("Couleur 3 ", (float*)&color3);

            if (ImGui::Button("Envoyer les couleurs a l'Arduino")) {
                // Buffer pour stocker notre chaîne de caractères formatée
                char buffer[128]; 
                
                // Formatage : "C:R1,G1,B1,R2,G2,B2,R3,G3,B3\n"
                snprintf(buffer, sizeof(buffer), "C:%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                    static_cast<int>(color1.x * 255.0f), static_cast<int>(color1.y * 255.0f), static_cast<int>(color1.z * 255.0f),
                    static_cast<int>(color2.x * 255.0f), static_cast<int>(color2.y * 255.0f), static_cast<int>(color2.z * 255.0f),
                    static_cast<int>(color3.x * 255.0f), static_cast<int>(color3.y * 255.0f), static_cast<int>(color3.z * 255.0f));

                // Envoi via le port série (Pense à vérifier que c'est bien COM3)
                SendToArduino(portCOM, std::string(buffer)); 
            }


            float alignement_x = ImGui::GetCursorPosX() + 300.0f;
            ImGui::Text("LED :");
            ImGui::SameLine(alignement_x); // Pour mettre l'interrupteur sur la même ligne que le texte
            if (ToggleButton("ToggleLED", &led_state)) {
                std::cout << "LED " << (led_state ? "ON" : "OFF") << std::endl;
            }


            ImGui::Text("Silence mod :");
            ImGui::SameLine(alignement_x); // Pour mettre l'interrupteur sur la même ligne que le texte
            if (ToggleButton("ToggleSilence", &silence_state)) {
                std::cout << "Silence " << (silence_state ? "ON" : "OFF") << std::endl;
            }


            ImGui::Text("Potentiomètre luminosité :");
            ImGui::SameLine(alignement_x);
            if (ToggleButton("TogglePot", &pot_state)) {


                if (pot_state) {
                    SendToArduino(portCOM, "POT_ON\n"); // Remplacez portCOM par le port série correct
                } else {
                    SendToArduino(portCOM, "POT_OFF\n");
                }
                std::cout << "Potentiomètre " << (pot_state ? "ON" : "OFF") << std::endl;
            }

            ImGui::Text("Application tournant à %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // Rendu final
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Gérer les fenêtres détachées (Docking)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }

        glfwSwapBuffers(window);
    }

    // Nettoyage
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
