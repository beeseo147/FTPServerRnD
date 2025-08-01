#pragma once

#include "CoreMinimal.h"
#include "FileManager.generated.h"

/**
 * 로컬 파일 시스템 관리를 담당하는 클래스
 */
UCLASS(BlueprintType)
class FILEUPLOAD_API UFileManager : public UObject
{
    GENERATED_BODY()

public:
    UFileManager();

    // 파일 검색
    UFUNCTION(BlueprintCallable, Category = "File Management")
    TArray<FString> FindFilesInDirectory(const FString& DirectoryPath, const TArray<FString>& FilePatterns);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    TArray<FString> FindFilesRecursively(const FString& DirectoryPath, const TArray<FString>& FilePatterns);

    // 디렉토리 관리
    UFUNCTION(BlueprintCallable, Category = "File Management")
    bool CreateDirectory(const FString& DirectoryPath);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    bool DirectoryExists(const FString& DirectoryPath);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    TArray<FString> GetSubDirectories(const FString& DirectoryPath);

    // 파일 정보
    UFUNCTION(BlueprintCallable, Category = "File Management")
    bool FileExists(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    int64 GetFileSize(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    FString GetFileName(const FString& FilePath);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    FString GetRelativePath(const FString& FullPath, const FString& BasePath);

    // 파일 복사/이동
    UFUNCTION(BlueprintCallable, Category = "File Management")
    bool CopyFile(const FString& SourcePath, const FString& DestPath);

    UFUNCTION(BlueprintCallable, Category = "File Management")
    bool MoveFile(const FString& SourcePath, const FString& DestPath);

    // 기본 파일 패턴
    UFUNCTION(BlueprintCallable, Category = "File Management")
    TArray<FString> GetDefaultFilePatterns();

private:
    // 플랫폼 파일 시스템 참조
    class IPlatformFile* PlatformFile;
}; 