# Standalone FTP Client Library

크로스 플랫폼 FTP 클라이언트 라이브러리입니다. 언리얼 엔진에 종속되지 않는 독립적인 C++ 라이브러리로, 다양한 FTP 클라이언트를 자동으로 감지하고 사용합니다.

## 🚀 주요 기능

- **다중 클라이언트 지원**: ftp.exe, curl.exe, PowerShell, WinSCP 자동 감지
- **크로스 플랫폼**: Windows, Linux, macOS 지원
- **비동기 작업**: 콜백 기반 진행 상황 모니터링
- **에러 처리**: 상세한 에러 메시지와 반환 코드
- **배치 작업**: 디렉토리 업로드/다운로드, 패턴 매칭
- **스레드 안전**: 멀티스레드 환경에서 안전한 사용

## 📋 지원하는 클라이언트

| 클라이언트 | Windows | Linux | macOS | 특징 |
|-----------|---------|-------|-------|------|
| **ftp.exe** | ✅ | ❌ | ❌ | Windows 기본, 안정적 |
| **curl.exe** | ✅ | ✅ | ✅ | 현대적, 기능 풍부 |
| **PowerShell** | ✅ | ❌ | ❌ | .NET 기반, 안정적 |
| **WinSCP** | ✅ | ❌ | ❌ | GUI 도구, 강력함 |

## 🛠️ 빌드 방법

### 요구사항

- CMake 3.16 이상
- C++17 호환 컴파일러
- Windows: Visual Studio 2019 이상
- Linux: GCC 7 이상 또는 Clang 6 이상

### 빌드 명령

```bash
# 디렉토리 생성
mkdir build && cd build

# CMake 설정
cmake ..

# 빌드
cmake --build . --config Release

# 예제 프로그램 빌드 (선택사항)
cmake --build . --config Release --target FtpClientExample
```

### 빌드 옵션

```bash
# 공유 라이브러리 빌드
cmake -DBUILD_SHARED_LIBS=ON ..

# 예제 프로그램 포함
cmake -DBUILD_EXAMPLES=ON ..

# 테스트 프로그램 포함
cmake -DBUILD_TESTS=ON ..
```

## 📖 사용 방법

### 기본 사용법

```cpp
#include "StandaloneFtpClient.h"
#include <iostream>

using namespace FtpClient;

int main()
{
    // FTP 클라이언트 생성
    auto client = CreateFtpClient();
    
    // 설정
    FtpConfig config;
    config.serverAddress = "localhost";
    config.username = "test";
    config.password = "test";
    config.port = 21;
    
    client->SetConfig(config);
    
    // 연결 테스트
    auto result = client->TestConnection();
    if (result.success)
    {
        std::cout << "연결 성공! 사용된 클라이언트: " << result.clientType << std::endl;
    }
    else
    {
        std::cout << "연결 실패: " << result.errorMessage << std::endl;
        return 1;
    }
    
    // 파일 업로드
    auto uploadResult = client->UploadFile("local_file.txt", "remote_file.txt");
    if (uploadResult.success)
    {
        std::cout << "업로드 성공!" << std::endl;
    }
    
    // 파일 다운로드
    auto downloadResult = client->DownloadFile("remote_file.txt", "downloaded_file.txt");
    if (downloadResult.success)
    {
        std::cout << "다운로드 성공!" << std::endl;
    }
    
    return 0;
}
```

### 콜백 사용법

```cpp
// 진행 상황 콜백
void ProgressCallback(const std::string& fileName, size_t bytesTransferred, size_t totalBytes)
{
    if (totalBytes > 0)
    {
        double percentage = (static_cast<double>(bytesTransferred) / totalBytes) * 100.0;
        std::cout << "Progress: " << fileName << " - " << percentage << "%" << std::endl;
    }
}

// 로그 콜백
void LogCallback(const std::string& message, bool isError)
{
    std::ostream& stream = isError ? std::cerr : std::cout;
    stream << (isError ? "[ERROR] " : "[INFO] ") << message << std::endl;
}

// 콜백 설정
client->SetProgressCallback(ProgressCallback);
client->SetLogCallback(LogCallback);
```

### 특정 클라이언트 사용

```cpp
// 사용 가능한 클라이언트 확인
auto availableClients = client->GetAvailableClients();
for (auto clientType : availableClients)
{
    std::cout << "Available client: " << static_cast<int>(clientType) << std::endl;
}

// 특정 클라이언트로 연결 테스트
auto curlResult = client->TestConnectionWithClient(ClientType::CurlExe);
if (curlResult.success)
{
    std::cout << "curl.exe 연결 성공!" << std::endl;
}
```

### 배치 작업

```cpp
// 디렉토리 업로드
auto uploadDirResult = client->UploadDirectory("./local_folder", "/remote_folder");

// 패턴 매칭 파일 업로드
std::vector<std::string> patterns = {".txt", ".log"};
auto patternUploadResult = client->UploadFilesWithPattern("./local_folder", "/remote_folder", patterns);

// 디렉토리 다운로드
auto downloadDirResult = client->DownloadDirectory("/remote_folder", "./downloaded_folder");
```

## 🔧 API 참조

### 주요 클래스

#### `StandaloneFtpClient`

메인 FTP 클라이언트 클래스입니다.

**생성자**
```cpp
StandaloneFtpClient();
```

**설정 메서드**
```cpp
void SetConfig(const FtpConfig& config);
void SetProgressCallback(ProgressCallback callback);
void SetLogCallback(LogCallback callback);
```

**연결 메서드**
```cpp
ConnectionResult TestConnection();
ConnectionResult TestConnectionWithClient(ClientType clientType);
```

**파일 작업 메서드**
```cpp
ConnectionResult UploadFile(const std::string& localFile, const std::string& remoteFile);
ConnectionResult DownloadFile(const std::string& remoteFile, const std::string& localFile);
ConnectionResult DeleteFile(const std::string& remoteFile);
ConnectionResult CreateDirectory(const std::string& remotePath);
ConnectionResult DeleteDirectory(const std::string& remotePath);
```

**유틸리티 메서드**
```cpp
std::vector<ClientType> GetAvailableClients();
bool IsClientAvailable(ClientType clientType);
std::string GetClientPath(ClientType clientType);
```

### 구조체

#### `FtpConfig`

FTP 서버 연결 설정을 담는 구조체입니다.

```cpp
struct FtpConfig
{
    std::string serverAddress;  // 서버 주소
    std::string username;       // 사용자명
    std::string password;       // 비밀번호
    int port = 21;             // 포트 번호
    int timeout = 30;          // 타임아웃 (초)
    bool passiveMode = true;   // 패시브 모드 사용 여부
    std::string workingDirectory; // 작업 디렉토리
};
```

#### `ConnectionResult`

FTP 작업 결과를 담는 구조체입니다.

```cpp
struct ConnectionResult
{
    bool success = false;           // 성공 여부
    std::string errorMessage;       // 에러 메시지
    std::string outputMessage;      // 출력 메시지
    int returnCode = 0;            // 반환 코드
    std::string clientType;        // 사용된 클라이언트 타입
};
```

#### `FileInfo`

파일 정보를 담는 구조체입니다.

```cpp
struct FileInfo
{
    std::string name;           // 파일명
    std::string path;           // 파일 경로
    bool isDirectory = false;   // 디렉토리 여부
    size_t size = 0;           // 파일 크기
    std::string lastModified;   // 마지막 수정 시간
    std::string permissions;    // 권한 정보
};
```

### 열거형

#### `ClientType`

지원하는 FTP 클라이언트 타입입니다.

```cpp
enum class ClientType
{
    FtpExe,     // Windows ftp.exe
    CurlExe,    // curl.exe
    PowerShell, // PowerShell
    WinSCP,     // WinSCP
    Custom      // 사용자 정의
};
```

## 🧪 테스트

### 테스트 실행

```bash
# 테스트 빌드
cmake -DBUILD_TESTS=ON ..
cmake --build . --config Release

# 테스트 실행
ctest --verbose
```

### 예제 프로그램 실행

```bash
# 예제 프로그램 빌드
cmake -DBUILD_EXAMPLES=ON ..
cmake --build . --config Release

# 예제 프로그램 실행
./FtpClientExample
```

## 📦 설치

### 시스템 설치

```bash
# 빌드 및 설치
cmake --build . --config Release --target install

# 또는
make install
```

### 패키지 생성

```bash
# 패키지 생성
cpack -G ZIP
cpack -G TGZ
```

## 🔍 문제 해결

### 일반적인 문제

1. **연결 실패**
   - FTP 서버가 실행 중인지 확인
   - 방화벽 설정 확인
   - 사용자명/비밀번호 확인

2. **클라이언트 감지 실패**
   - 시스템에 해당 클라이언트가 설치되어 있는지 확인
   - PATH 환경변수 설정 확인

3. **권한 오류**
   - 파일/디렉토리 권한 확인
   - 관리자 권한으로 실행

### 디버깅

```cpp
// 상세한 로그 활성화
client->SetLogCallback([](const std::string& message, bool isError) {
    std::cout << (isError ? "[ERROR] " : "[DEBUG] ") << message << std::endl;
});

// 연결 결과 상세 확인
auto result = client->TestConnection();
std::cout << "Success: " << result.success << std::endl;
std::cout << "Error: " << result.errorMessage << std::endl;
std::cout << "Output: " << result.outputMessage << std::endl;
std::cout << "Return Code: " << result.returnCode << std::endl;
std::cout << "Client Type: " << result.clientType << std::endl;
```

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 `LICENSE` 파일을 참조하세요.

## 🤝 기여하기

1. 이 저장소를 포크합니다
2. 기능 브랜치를 생성합니다 (`git checkout -b feature/amazing-feature`)
3. 변경사항을 커밋합니다 (`git commit -m 'Add amazing feature'`)
4. 브랜치에 푸시합니다 (`git push origin feature/amazing-feature`)
5. Pull Request를 생성합니다

## 📞 지원

- **이슈 리포트**: GitHub Issues 사용
- **문서**: 이 README와 예제 코드 참조
- **이메일**: your.email@example.com

## 🔄 변경 이력

### v1.0.0
- 초기 릴리스
- 다중 클라이언트 지원
- 크로스 플랫폼 지원
- 기본 FTP 작업 구현

## 🙏 감사의 말

- Windows ftp.exe 개발팀
- curl 프로젝트 팀
- PowerShell 개발팀
- WinSCP 개발팀 