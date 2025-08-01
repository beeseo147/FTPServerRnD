// Copyright Epic Games, Inc. All Rights Reserved.

#include "FileUpLoad.h"
#include "FileUpLoadStyle.h"
#include "FileUpLoadCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Containers/Set.h"
#include "Misc/DateTime.h"

// 탭 이름 상수들
static const FName FileUpLoadTabName(TEXT("FileUpLoad"));
static const FName FtpClientTabName(TEXT("FtpClient"));
static const FName FileManagerTabName(TEXT("FileManager"));
static const FName UploadHistoryTabName(TEXT("UploadHistory"));

#define LOCTEXT_NAMESPACE "FFileUpLoadModule"

// FTP 사용자 설정 구조체
struct FFtpUserConfig
{
    FString Username;
    FString Password;
    FString HomeDirectory;
    TArray<FString> Permissions;

    FFtpUserConfig()
    {
        Username = TEXT("");
        Password = TEXT("");
        HomeDirectory = TEXT("/");
        Permissions = { TEXT("Read") };
    }
};

// 보안 설정 구조체
struct FFtpSecurityConfig
{
    int32 MaxLoginAttempts = 5;
    int32 LockoutDuration = 300; // 5분
    bool EnableLogging = true;

    FFtpSecurityConfig()
    {
        MaxLoginAttempts = 5;
        LockoutDuration = 300;
        EnableLogging = true;
    }
};

// 전역 변수들
static TArray<FFtpUserConfig> GFtpUsers;
static FFtpSecurityConfig GFtpSecurityConfig;
static FString GServerAddress = TEXT("192.168.0.35");
static int32 GServerPort = 21;
static TMap<FString, int32> GLoginAttempts;
static TMap<FString, FDateTime> GLockoutTimes;

//2025.07.24 KDG
//플러그인이 로드될 때 호출되는 초기화 함수
//여기서 ToolMenus 콜백을 등록해 툴바를 확장하는 작업을 수행합니다.
void FFileUpLoadModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FFileUpLoadStyle::Initialize();
	FFileUpLoadStyle::ReloadTextures();

	FFileUpLoadCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FFileUpLoadCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FFileUpLoadModule::PluginButtonClicked),
		FCanExecuteAction());

	// 탭 매니저 초기화 - 간단한 방식으로 변경
	
	// 각 탭 등록
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FileUpLoadTabName, FOnSpawnTab::CreateRaw(this, &FFileUpLoadModule::SpawnFileUploadTab))
		.SetDisplayName(LOCTEXT("FFileUpLoadTabTitle", "File Upload"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
		
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FtpClientTabName, FOnSpawnTab::CreateRaw(this, &FFileUpLoadModule::SpawnFtpClientTab))
		.SetDisplayName(LOCTEXT("FFtpClientTabTitle", "FTP Client"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
		
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FileManagerTabName, FOnSpawnTab::CreateRaw(this, &FFileUpLoadModule::SpawnFileManagerTab))
		.SetDisplayName(LOCTEXT("FFileManagerTabTitle", "File Manager"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
		
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UploadHistoryTabName, FOnSpawnTab::CreateRaw(this, &FFileUpLoadModule::SpawnUploadHistoryTab))
		.SetDisplayName(LOCTEXT("FUploadHistoryTabTitle", "Upload History"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FFileUpLoadModule::RegisterMenus));

	// FTP 시스템 초기화
	InitializeFtpSystem();

	// FTP 시스템 초기화
	InitializeFtpSystem();
}

void FFileUpLoadModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// 탭 스폰어 등록 해제
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FileUpLoadTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FtpClientTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FileManagerTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UploadHistoryTabName);

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FFileUpLoadStyle::Shutdown();

	FFileUpLoadCommands::Unregister();
}

void FFileUpLoadModule::InitializeFtpSystem()
{
	// 기본 사용자 설정
	FFtpUserConfig TestUser;
	TestUser.Username = TEXT("test");
	TestUser.Password = TEXT("test");
	TestUser.HomeDirectory = TEXT("/test");
	TestUser.Permissions = { TEXT("Read"), TEXT("Write"), TEXT("Delete") };
	GFtpUsers.Add(TestUser);

	FFtpUserConfig AdminUser;
	AdminUser.Username = TEXT("admin");
	AdminUser.Password = TEXT("admin");
	AdminUser.HomeDirectory = TEXT("/admin");
	AdminUser.Permissions = { TEXT("Read"), TEXT("Write"), TEXT("Delete"), TEXT("Admin") };
	GFtpUsers.Add(AdminUser);

	// 보안 설정
	GFtpSecurityConfig.MaxLoginAttempts = 5;
	GFtpSecurityConfig.LockoutDuration = 300; // 5분
	GFtpSecurityConfig.EnableLogging = true;

	// 서버 설정
	GServerAddress = TEXT("192.168.0.35");
	GServerPort = 21;

	UE_LOG(LogTemp, Log, TEXT("FTP System initialized successfully"));
}

// 전방 선언
bool IsUserLocked(const FString& Username);
FFtpUserConfig* GetUser(const FString& Username);
void RecordLoginAttempt(const FString& Username, bool bSuccess, const FString& IpAddress);

// 로그 출력 함수
void LogFtpMessage(const FString& Message, bool bIsError = false)
{
	if (GFtpSecurityConfig.EnableLogging)
	{
		if (bIsError)
		{
			UE_LOG(LogTemp, Error, TEXT("FTP System: %s"), *Message);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("FTP System: %s"), *Message);
		}
	}
}

// 사용자 인증
bool AuthenticateUser(const FString& Username, const FString& Password)
{
	// 계정 잠금 확인
	if (IsUserLocked(Username))
	{
		LogFtpMessage(FString::Printf(TEXT("Login attempt on locked account: %s"), *Username), true);
		return false;
	}

	FFtpUserConfig* User = GetUser(Username);
	if (User == nullptr)
	{
		RecordLoginAttempt(Username, false, TEXT("unknown"));
		return false;
	}

	bool bIsValid = User->Password.Equals(Password, ESearchCase::CaseSensitive);
	RecordLoginAttempt(Username, bIsValid, TEXT("unknown"));

	if (bIsValid)
	{
		// 성공 시 로그인 시도 횟수 초기화
		GLoginAttempts.Remove(Username);
		GLockoutTimes.Remove(Username);
	}

	return bIsValid;
}

// 사용자 정보 조회
FFtpUserConfig* GetUser(const FString& Username)
{
	for (FFtpUserConfig& User : GFtpUsers)
	{
		if (User.Username.Equals(Username, ESearchCase::IgnoreCase))
		{
			return &User;
		}
	}
	return nullptr;
}

// 사용자 권한 확인
bool HasPermission(const FString& Username, const FString& Permission)
{
	FFtpUserConfig* User = GetUser(Username);
	if (User == nullptr)
		return false;

	return User->Permissions.Contains(Permission);
}

// 로그인 시도 기록
void RecordLoginAttempt(const FString& Username, bool bSuccess, const FString& IpAddress)
{
	if (!bSuccess)
	{
		int32* AttemptsPtr = GLoginAttempts.Find(Username);
		int32 Attempts = AttemptsPtr ? *AttemptsPtr + 1 : 1;
		GLoginAttempts.Add(Username, Attempts);
		
		LogFtpMessage(FString::Printf(TEXT("Login failed: %s from %s, attempts: %d"), *Username, *IpAddress, Attempts), true);

		if (Attempts >= GFtpSecurityConfig.MaxLoginAttempts)
		{
			FDateTime LockoutTime = FDateTime::UtcNow() + FTimespan::FromSeconds(GFtpSecurityConfig.LockoutDuration);
			GLockoutTimes.Add(Username, LockoutTime);
			
			LogFtpMessage(FString::Printf(TEXT("Account locked: %s until %s"), *Username, *LockoutTime.ToString()), true);
		}
	}
	else
	{
		LogFtpMessage(FString::Printf(TEXT("Login successful: %s from %s"), *Username, *IpAddress), false);
	}
}

// 사용자 잠금 확인
bool IsUserLocked(const FString& Username)
{
	FDateTime* LockoutTimePtr = GLockoutTimes.Find(Username);
	if (LockoutTimePtr)
	{
		if (FDateTime::UtcNow() < *LockoutTimePtr)
		{
			return true;
		}
		else
		{
			// 잠금 시간이 지났으면 잠금 해제
			GLockoutTimes.Remove(Username);
			GLoginAttempts.Remove(Username);
		}
	}

	return false;
}

// CURL 명령어 실행
bool ExecuteCurlCommand(const FString& Command, FString& Output, FString& Error)
{
	int32 ReturnCode;
	FString StdOut, StdErr;
	
	bool bSuccess = FPlatformProcess::ExecProcess(TEXT("curl.exe"), *Command, &ReturnCode, &StdOut, &StdErr);
	
	Output = StdOut;
	Error = StdErr;
	
	return bSuccess && ReturnCode == 0;
}

// FTP URL 생성
FString CreateFtpUrl(const FString& Username, const FString& RemotePath)
{
	FFtpUserConfig* User = GetUser(Username);
	if (!User)
		return TEXT("");

	FString BaseUrl = FString::Printf(TEXT("ftp://%s:%d"), *GServerAddress, GServerPort);
	
	// 루트 디렉토리인 경우
	if (RemotePath.IsEmpty() || RemotePath == TEXT("/"))
	{
		return BaseUrl + TEXT("/");
	}
	
	if (RemotePath.StartsWith(TEXT("/")))
	{
		return BaseUrl + RemotePath;
	}
	else
	{
		return BaseUrl + TEXT("/") + RemotePath;
	}
}

// FTP 파일 업로드
bool UploadFile(const FString& Username, const FString& LocalPath, const FString& RemotePath)
{
	if (!HasPermission(Username, TEXT("Write")))
	{
		LogFtpMessage(FString::Printf(TEXT("Upload failed: User %s lacks write permission"), *Username), true);
		return false;
	}

	if (!FPaths::FileExists(LocalPath))
	{
		LogFtpMessage(FString::Printf(TEXT("Upload failed: Local file does not exist: %s"), *LocalPath), true);
		return false;
	}

	FFtpUserConfig* User = GetUser(Username);
	if (!User)
		return false;

	FString FtpUrl = CreateFtpUrl(Username, RemotePath);
	FString Command = FString::Printf(TEXT("-T \"%s\" \"%s\" --user %s:%s --ftp-pasv --ftp-create-dirs"), 
		*LocalPath, *FtpUrl, *User->Username, *User->Password);

	FString Output, Error;
	bool bSuccess = ExecuteCurlCommand(Command, Output, Error);

	if (bSuccess)
	{
		LogFtpMessage(FString::Printf(TEXT("Upload successful: %s -> %s"), *LocalPath, *RemotePath), false);
	}
	else
	{
		LogFtpMessage(FString::Printf(TEXT("Upload failed: %s"), *Error), true);
	}

	return bSuccess;
}

// FTP 파일 다운로드
bool DownloadFile(const FString& Username, const FString& RemotePath, const FString& LocalPath)
{
	if (!HasPermission(Username, TEXT("Read")))
	{
		LogFtpMessage(FString::Printf(TEXT("Download failed: User %s lacks read permission"), *Username), true);
		return false;
	}

	FFtpUserConfig* User = GetUser(Username);
	if (!User)
		return false;

	FString FtpUrl = CreateFtpUrl(Username, RemotePath);
	FString Command = FString::Printf(TEXT("\"%s\" --user %s:%s -o \"%s\" --ftp-pasv"), 
		*FtpUrl, *User->Username, *User->Password, *LocalPath);

	FString Output, Error;
	bool bSuccess = ExecuteCurlCommand(Command, Output, Error);

	if (bSuccess)
	{
		LogFtpMessage(FString::Printf(TEXT("Download successful: %s -> %s"), *RemotePath, *LocalPath), false);
	}
	else
	{
		LogFtpMessage(FString::Printf(TEXT("Download failed: %s"), *Error), true);
	}

	return bSuccess;
}

// FTP 파일 목록 조회
bool GetFileList(const FString& Username, const FString& RemotePath, TArray<FString>& FileList)
{
	if (!HasPermission(Username, TEXT("Read")))
	{
		LogFtpMessage(FString::Printf(TEXT("GetFileList failed: User %s lacks read permission"), *Username), true);
		return false;
	}

	FFtpUserConfig* User = GetUser(Username);
	if (!User)
		return false;

	// 루트 디렉토리부터 시작
	FString FtpUrl = FString::Printf(TEXT("ftp://%s:%d/"), *GServerAddress, GServerPort);
	FString Command = FString::Printf(TEXT("\"%s\" --user %s:%s --silent --show-error --ftp-pasv"), 
		*FtpUrl, *User->Username, *User->Password);

	FString Output, Error;
	bool bSuccess = ExecuteCurlCommand(Command, Output, Error);

	if (bSuccess)
	{
		// 출력을 줄 단위로 분리
		TArray<FString> Lines;
		Output.ParseIntoArray(Lines, TEXT("\n"), true);
		
		for (const FString& Line : Lines)
		{
			if (!Line.IsEmpty())
			{
				FileList.Add(Line);
			}
		}

		LogFtpMessage(FString::Printf(TEXT("GetFileList successful: %d files found"), FileList.Num()), false);
	}
	else
	{
		LogFtpMessage(FString::Printf(TEXT("GetFileList failed: %s"), *Error), true);
	}

	return bSuccess;
}

// 연결 테스트
bool TestConnection(const FString& Username)
{
	FFtpUserConfig* User = GetUser(Username);
	if (!User)
	{
		LogFtpMessage(FString::Printf(TEXT("TestConnection failed: Invalid user %s"), *Username), true);
		return false;
	}

	// 더 간단한 연결 테스트 - 루트 디렉토리만 확인
	FString FtpUrl = FString::Printf(TEXT("ftp://%s:%d/"), *GServerAddress, GServerPort);
	FString Command = FString::Printf(TEXT("\"%s\" --user %s:%s --connect-timeout 10 --max-time 30 --silent --show-error --ftp-pasv --ftp-create-dirs"), 
		*FtpUrl, *User->Username, *User->Password);

	FString Output, Error;
	bool bSuccess = ExecuteCurlCommand(Command, Output, Error);

	if (bSuccess)
	{
		LogFtpMessage(FString::Printf(TEXT("Connection test successful for user: %s"), *Username), false);
	}
	else
	{
		LogFtpMessage(FString::Printf(TEXT("Connection test failed for user %s: %s"), *Username, *Error), true);
	}

	return bSuccess;
}

void UploadSpecificFolder(const FString& LocalFolder, const FString& RemoteBaseDir, const FString& Server, const FString& User, const FString& Pass)
{
    UE_LOG(LogTemp, Log, TEXT("특정 폴더 업로드 시작: %s"), *LocalFolder);
    
    // 사용자 인증
    if (!AuthenticateUser(User, Pass))
    {
        UE_LOG(LogTemp, Error, TEXT("사용자 인증 실패: %s"), *User);
        return;
    }
    
    TArray<FString> AllFiles;
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // 특정 폴더의 모든 파일 찾기
    PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*"));
    
    UE_LOG(LogTemp, Log, TEXT("총 %d개 파일 발견"), AllFiles.Num());
    
    int32 SuccessCount = 0;
    int32 FailCount = 0;
    
    for (const FString& LocalFile : AllFiles)
    {
        // 상대 경로 계산
        FString RelativePath = LocalFile;
        FPaths::MakePathRelativeTo(RelativePath, *LocalFolder);
        
        // 원격 경로 생성
        FString RemoteFile = RemoteBaseDir / RelativePath;
        
        UE_LOG(LogTemp, Log, TEXT("업로드 중: %s -> %s"), *LocalFile, *RemoteFile);
        
        if (UploadFile(User, LocalFile, RemoteFile)) {
            SuccessCount++;
            UE_LOG(LogTemp, Log, TEXT("업로드 성공: %s"), *RelativePath);
        } else {
            FailCount++;
            UE_LOG(LogTemp, Error, TEXT("업로드 실패: %s"), *RelativePath);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("특정 폴더 업로드 완료: 성공 %d개, 실패 %d개"), SuccessCount, FailCount);
}

void UploadFolderStructure(const FString& LocalFolder, const FString& RemoteBaseDir, const FString& Server, const FString& User, const FString& Pass)
{
    UE_LOG(LogTemp, Log, TEXT("폴더 구조 업로드 시작: %s"), *LocalFolder);
    
    // 사용자 인증
    if (!AuthenticateUser(User, Pass))
    {
        UE_LOG(LogTemp, Error, TEXT("사용자 인증 실패: %s"), *User);
        return;
    }
    
    TArray<FString> AllFiles;
    TSet<FString> AllDirectories; // 중복 제거를 위해 TSet 사용
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // 모든 파일 찾기 (여러 패턴으로 시도)
    PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*.*"));
    
    // 만약 파일이 없으면 다른 패턴 시도
    if (AllFiles.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("*. 패턴으로 재시도"));
        PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*"));
    }
    
    // 여전히 파일이 없으면 모든 확장자 시도
    if (AllFiles.Num() == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("모든 확장자로 재시도"));
        PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*.txt"));
        PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*.uasset"));
        PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*.umap"));
        PlatformFile.FindFilesRecursively(AllFiles, *LocalFolder, TEXT("*.ini"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("파일 검색 결과: %d개 파일 발견"), AllFiles.Num());
    
    // 발견된 파일들 로그 출력
    for (int32 i = 0; i < AllFiles.Num(); i++)
    {
        UE_LOG(LogTemp, Log, TEXT("파일 %d: %s"), i, *AllFiles[i]);
    }
    
    // 파일 경로에서 폴더 경로 추출
    for (const FString& FilePath : AllFiles)
    {
        FString DirectoryPath = FPaths::GetPath(FilePath);
        AllDirectories.Add(DirectoryPath);
    }
    
    UE_LOG(LogTemp, Log, TEXT("총 %d개 파일, %d개 폴더 발견"), AllFiles.Num(), AllDirectories.Num());
    
    int32 SuccessCount = 0;
    int32 FailCount = 0;
    
    // 1단계: 모든 폴더 경로 준비
    for (const FString& LocalDir : AllDirectories)
    {
        FString RelativePath = LocalDir;
        FPaths::MakePathRelativeTo(RelativePath, *LocalFolder);
        FString RemoteDir = RemoteBaseDir / RelativePath;
        
        UE_LOG(LogTemp, Log, TEXT("폴더 경로 준비: %s -> %s"), *LocalDir, *RemoteDir);
    }
    
    // 2단계: 모든 파일 업로드
    for (const FString& LocalFile : AllFiles)
    {
        // 상대 경로 계산
        FString RelativePath = LocalFile;
        FPaths::MakePathRelativeTo(RelativePath, *LocalFolder);
        
        // 원격 경로 생성
        FString RemoteFile = RemoteBaseDir / RelativePath;
        
        UE_LOG(LogTemp, Log, TEXT("업로드 중: %s -> %s"), *LocalFile, *RemoteFile);
        
        if (UploadFile(User, LocalFile, RemoteFile)) {
            SuccessCount++;
            UE_LOG(LogTemp, Log, TEXT("업로드 성공: %s"), *RelativePath);
        } else {
            FailCount++;
            UE_LOG(LogTemp, Error, TEXT("업로드 실패: %s"), *RelativePath);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("폴더 구조 업로드 완료: 성공 %d개, 실패 %d개"), SuccessCount, FailCount);
}

void UploadFromFtpServer(const FString& RemotePath, const FString& LocalPath, const FString& Server, const FString& User, const FString& Pass)
{
    UE_LOG(LogTemp, Log, TEXT("=== FTP 서버에서 파일 다운로드 시작 ==="));
    UE_LOG(LogTemp, Log, TEXT("FTP 서버 경로: %s"), *RemotePath);
    UE_LOG(LogTemp, Log, TEXT("로컬 저장 경로: %s"), *LocalPath);
    
    // 사용자 인증
    if (!AuthenticateUser(User, Pass))
    {
        UE_LOG(LogTemp, Error, TEXT("사용자 인증 실패: %s"), *User);
        return;
    }
    
    // 1단계: FTP 서버에서 파일 목록 가져오기
    TArray<FString> FileList;
    if (!GetFileList(User, RemotePath, FileList)) {
        UE_LOG(LogTemp, Error, TEXT("FTP 파일 목록 가져오기 실패"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("FTP 서버에서 %d개 파일 발견"), FileList.Num());
    
    // 발견된 파일들 로그 출력
    for (int32 i = 0; i < FileList.Num(); i++) {
        UE_LOG(LogTemp, Log, TEXT("파일 %d: %s"), i, *FileList[i]);
    }
    
    // 2단계: 각 파일 다운로드
    int32 SuccessCount = 0;
    int32 FailCount = 0;
    
    for (const FString& RemoteFile : FileList) {
        FString FullRemotePath = RemotePath / RemoteFile;
        FString FullLocalPath = LocalPath / RemoteFile;
        
        UE_LOG(LogTemp, Log, TEXT("다운로드 중: %s -> %s"), *FullRemotePath, *FullLocalPath);
        
        if (DownloadFile(User, FullRemotePath, FullLocalPath)) {
            SuccessCount++;
            UE_LOG(LogTemp, Log, TEXT("다운로드 성공: %s"), *RemoteFile);
        } else {
            FailCount++;
            UE_LOG(LogTemp, Error, TEXT("다운로드 실패: %s"), *RemoteFile);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("=== FTP 다운로드 완료: 성공 %d개, 실패 %d개 ==="), SuccessCount, FailCount);
}

void UploadToFtpServer(const FString& LocalPath, const FString& RemotePath, const FString& Server, const FString& User, const FString& Pass)
{
    UE_LOG(LogTemp, Log, TEXT("=== FTP 서버로 파일 업로드 시작 ==="));
    UE_LOG(LogTemp, Log, TEXT("로컬 경로: %s"), *LocalPath);
    UE_LOG(LogTemp, Log, TEXT("FTP 서버 경로: %s"), *RemotePath);
    
    // 사용자 인증
    if (!AuthenticateUser(User, Pass))
    {
        UE_LOG(LogTemp, Error, TEXT("사용자 인증 실패: %s"), *User);
        return;
    }
    
    // 1단계: 로컬 파일 목록 가져오기 (개선된 방법)
    TArray<FString> AllFiles;
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // 디렉토리 존재 확인
    if (!FPaths::DirectoryExists(LocalPath)) {
        UE_LOG(LogTemp, Error, TEXT("로컬 경로가 존재하지 않습니다: %s"), *LocalPath);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("로컬 경로 존재 확인됨: %s"), *LocalPath);
    
    // 모든 파일을 찾기 위해 여러 방법 시도
    TArray<FString> Patterns = {TEXT("*"), TEXT("*.*"), TEXT("*.txt"), TEXT("*.uasset"), TEXT("*.umap"), TEXT("*.ini"), TEXT("*.bat"), TEXT("*.md")};
    
    for (const FString& Pattern : Patterns) {
        TArray<FString> PatternFiles;
        PlatformFile.FindFilesRecursively(PatternFiles, *LocalPath, *Pattern);
        UE_LOG(LogTemp, Log, TEXT("패턴 %s: %d개 파일 발견"), *Pattern, PatternFiles.Num());
        
        // 각 파일의 전체 경로 출력
        for (const FString& File : PatternFiles) {
            UE_LOG(LogTemp, Log, TEXT("  발견된 파일: %s"), *File);
        }
        
        AllFiles.Append(PatternFiles);
    }
    
    // 중복 제거
    TSet<FString> UniqueFiles(AllFiles);
    AllFiles = UniqueFiles.Array();
    
    UE_LOG(LogTemp, Log, TEXT("총 %d개 고유 파일 발견"), AllFiles.Num());
    
    // 발견된 파일들 로그 출력
    for (int32 i = 0; i < AllFiles.Num(); i++) {
        UE_LOG(LogTemp, Log, TEXT("파일 %d: %s"), i, *AllFiles[i]);
    }
    
    // 파일이 없으면 대안 방법 시도
    if (AllFiles.Num() == 0) {
        UE_LOG(LogTemp, Warning, TEXT("기본 패턴으로 파일을 찾지 못했습니다. 대안 방법 시도..."));
        
        // 대안 1: 디렉토리 내 모든 항목 나열
        TArray<FString> AllItems;
        PlatformFile.FindFiles(AllItems, *LocalPath, TEXT("*"));
        UE_LOG(LogTemp, Log, TEXT("대안 1 - 모든 항목: %d개 발견"), AllItems.Num());
        
        for (const FString& Item : AllItems) {
            UE_LOG(LogTemp, Log, TEXT("  항목: %s"), *Item);
        }
        
        // 대안 2: 하위 디렉토리 검색 (수동으로 구현)
        TArray<FString> SubDirs;
        PlatformFile.FindFiles(SubDirs, *LocalPath, TEXT("*"));
        
        // 디렉토리만 필터링
        TArray<FString> Directories;
        for (const FString& Item : SubDirs) {
            if (PlatformFile.DirectoryExists(*Item)) {
                Directories.Add(Item);
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("대안 2 - 하위 디렉토리: %d개 발견"), Directories.Num());
        
        for (const FString& SubDir : Directories) {
            UE_LOG(LogTemp, Log, TEXT("  하위 디렉토리: %s"), *SubDir);
            
            // 각 하위 디렉토리에서 파일 검색
            TArray<FString> SubDirFiles;
            PlatformFile.FindFilesRecursively(SubDirFiles, *SubDir, TEXT("*"));
            UE_LOG(LogTemp, Log, TEXT("    %s에서 %d개 파일 발견"), *SubDir, SubDirFiles.Num());
            
            for (const FString& File : SubDirFiles) {
                UE_LOG(LogTemp, Log, TEXT("      파일: %s"), *File);
                AllFiles.Add(File);
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("대안 방법 후 총 %d개 파일 발견"), AllFiles.Num());
    }
    
    // 2단계: 각 파일을 FTP 서버로 업로드
    int32 SuccessCount = 0;
    int32 FailCount = 0;
    
    for (const FString& LocalFile : AllFiles) {
        // 상대 경로 계산
        FString RelativePath = LocalFile;
        FPaths::MakePathRelativeTo(RelativePath, *LocalPath);
        
        // 원격 경로 생성
        FString RemoteFile = RemotePath / RelativePath;
        
        UE_LOG(LogTemp, Log, TEXT("업로드 중: %s -> %s"), *LocalFile, *RemoteFile);
        
        if (UploadFile(User, LocalFile, RemoteFile)) {
            SuccessCount++;
            UE_LOG(LogTemp, Log, TEXT("업로드 성공: %s"), *RelativePath);
        } else {
            FailCount++;
            UE_LOG(LogTemp, Error, TEXT("업로드 실패: %s"), *RelativePath);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("=== FTP 업로드 완료: 성공 %d개, 실패 %d개 ==="), SuccessCount, FailCount);
}

void FFileUpLoadModule::PluginButtonClicked()
{
	// 메인 File Upload 탭만 열기
	FGlobalTabmanager::Get()->TryInvokeTab(FileUpLoadTabName);
}

//2025.07.24 KDG : 에디터 툴바를 확장하고,드롭다운과 각종 버튼을 추가하는 함수
void FFileUpLoadModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			
			// File Upload 서브메뉴 추가 (Cursor 스타일)
			FToolMenuEntry& SubMenuEntry = Section.AddSubMenu(
				NAME_None,
				LOCTEXT("FileUploadSubMenu", "File Upload"),
				LOCTEXT("FileUploadSubMenuToolTip", "Open File Upload windows"),
				FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
				{
					FToolMenuSection& SubSection = SubMenu->FindOrAddSection("FileUploadSection");
					
					// File Upload 탭
					SubSection.AddMenuEntry(
						"FileUpload",
						LOCTEXT("FileUploadTab", "File Upload"),
						LOCTEXT("FileUploadTabToolTip", "Open File Upload tab"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([]()
						{
							FGlobalTabmanager::Get()->TryInvokeTab(FileUpLoadTabName);
						}))
					);
					
					// FTP Client 탭
					SubSection.AddMenuEntry(
						"FtpClient",
						LOCTEXT("FtpClientTab", "FTP Client"),
						LOCTEXT("FtpClientTabToolTip", "Open FTP Client tab"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([]()
						{
							FGlobalTabmanager::Get()->TryInvokeTab(FtpClientTabName);
						}))
					);
					
					// File Manager 탭
					SubSection.AddMenuEntry(
						"FileManager",
						LOCTEXT("FileManagerTab", "File Manager"),
						LOCTEXT("FileManagerTabToolTip", "Open File Manager tab"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([]()
						{
							FGlobalTabmanager::Get()->TryInvokeTab(FileManagerTabName);
						}))
					);
					
					// Upload History 탭
					SubSection.AddMenuEntry(
						"UploadHistory",
						LOCTEXT("UploadHistoryTab", "Upload History"),
						LOCTEXT("UploadHistoryTabToolTip", "Open Upload History tab"),
						FSlateIcon(),
						FUIAction(FExecuteAction::CreateLambda([]()
						{
							FGlobalTabmanager::Get()->TryInvokeTab(UploadHistoryTabName);
						}))
					);
				})
			);
		}
	}
}

// FileUpLoad 버튼 별 기능
TSharedRef<SDockTab> FFileUpLoadModule::SpawnFileUploadTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FileUploadTitle", "File Upload"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(SButton)
				.Text(LOCTEXT("TestFtpButton", "Test FTP Upload"))
				.OnClicked_Lambda([]()
				{
					FString Server = TEXT("192.168.0.35");
					FString User = TEXT("test");
					FString Pass = TEXT("test");

					UE_LOG(LogTemp, Log, TEXT("=== curl.exe 기반 FTP 시스템 테스트 ==="));
					
					// 1. FTP 서버 연결 테스트
					UE_LOG(LogTemp, Log, TEXT("=== 1단계: FTP 서버 연결 테스트 ==="));
					if (TestConnection(User)) {
						UE_LOG(LogTemp, Log, TEXT("FTP 서버 연결 성공"));
					} else {
						UE_LOG(LogTemp, Error, TEXT("FTP 서버 연결 실패"));
						// 실패해도 계속 진행 (로컬 파일 테스트)
					}
					
					// 2. 로컬 파일 검색 테스트
					UE_LOG(LogTemp, Log, TEXT("=== 2단계: 로컬 파일 검색 테스트 ==="));
					FString ContentDir = FPaths::ProjectContentDir();
					UE_LOG(LogTemp, Log, TEXT("Content 디렉토리: %s"), *ContentDir);
					
					// Content 디렉토리 존재 확인
					if (FPaths::DirectoryExists(ContentDir)) {
						UE_LOG(LogTemp, Log, TEXT("Content 디렉토리 존재 확인됨"));
						
						// 간단한 파일 검색 테스트 (재귀적 검색)
						IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
						TArray<FString> TestFiles;
						PlatformFile.FindFilesRecursively(TestFiles, *ContentDir, TEXT("*"));
						UE_LOG(LogTemp, Log, TEXT("Content 디렉토리에서 %d개 항목 발견 (재귀적 검색)"), TestFiles.Num());
						
						for (const FString& Item : TestFiles) {
							UE_LOG(LogTemp, Log, TEXT("  항목: %s"), *Item);
						}
					} else {
						UE_LOG(LogTemp, Error, TEXT("Content 디렉토리가 존재하지 않습니다"));
					}
					
					// 3. 로컬 콘텐츠를 FTP 서버로 업로드
					UE_LOG(LogTemp, Log, TEXT("=== 3단계: FTP 업로드 테스트 ==="));
					FString RemoteBaseDir = TEXT("upload/content");
					UploadToFtpServer(ContentDir, RemoteBaseDir, Server, User, Pass);
					
					// 4. FTP 서버에서 파일 다운로드 (테스트)
					UE_LOG(LogTemp, Log, TEXT("=== 4단계: FTP 다운로드 테스트 ==="));
					FString DownloadDir = FPaths::ProjectSavedDir() / TEXT("ftp_download");
					UploadFromFtpServer(TEXT("upload/content"), DownloadDir, Server, User, Pass);
					
					UE_LOG(LogTemp, Log, TEXT("=== curl.exe 기반 FTP 시스템 테스트 완료 ==="));
					
					return FReply::Handled();
				})
			]
		];
}

TSharedRef<SDockTab> FFileUpLoadModule::SpawnFtpClientTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FtpClientTitle", "FTP Client"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ServerLabel", "Server:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(5)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(TEXT("192.168.0.35")))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("UserLabel", "User:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(5)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(TEXT("test")))
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PasswordLabel", "Password:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(5)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(TEXT("test")))
				]
			]
		];
}

TSharedRef<SDockTab> FFileUpLoadModule::SpawnFileManagerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FileManagerTitle", "File Manager"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("FileManagerContent", "File Manager content will be displayed here."))
			]
		];
}

TSharedRef<SDockTab> FFileUpLoadModule::SpawnUploadHistoryTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UploadHistoryTitle", "Upload History"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(10)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UploadHistoryContent", "Upload history will be displayed here."))
			]
		];
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFileUpLoadModule, FileUpLoad)