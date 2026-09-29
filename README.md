# Petrol Receipt Analyzer

A cross-platform C++ desktop application featuring a multi-screen interface built using the **wxWidgets** GUI framework. The application relies on a vision LLM model to analyze and extract informations from the provided image. The structure provided utilised a locally hosted LLM (Gemma4:26B model) using the OLLAMA platform to run the backend process.

## Prerequisites

To compile and run this application, you must have:

- A C++ compiler supporting at least **C++17**
- **wxWidgets** (v3.3.3)
- **nlohmann_json** (v3.12.0)
- **CMake** (v3.16 or newer)

## Sample Interface (on Windows)

### Application Interface
<img width="796" height="609" alt="FrontUI" src="https://github.com/user-attachments/assets/2148407e-6aab-460c-94c4-3ad590248daa" />

### End Result
<img width="1180" height="626" alt="Info Extracted" src="https://github.com/user-attachments/assets/0db08e1f-5887-44ae-b79f-18c051ad737d" />
