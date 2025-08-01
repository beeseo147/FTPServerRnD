#include "FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

UFileManager::UFileManager()
{
    PlatformFile = &FPlatformFileManager::Get().GetPlatformFile();
}

TArray<FString> UFileManager::FindFilesInDirectory(const FString& DirectoryPath, const TArray<FString>& FilePatterns)
{
    TArray<FString> FoundFiles;
    
    if (!PlatformFile->DirectoryExists(*DirectoryPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("디렉토리가 존재하지 않습니다: %s"), *DirectoryPath);
        return FoundFiles;
    }

    for (const FString& Pattern : FilePatterns)
    {
        TArray<FString> PatternFiles;
        PlatformFile->FindFiles(PatternFiles, *DirectoryPath, *Pattern);
        FoundFiles.Append(PatternFiles);
    }

    return FoundFiles;
}

TArray<FString> UFileManager::FindFilesRecursively(const FString& DirectoryPath, const TArray<FString>& FilePatterns)
{
    TArray<FString> FoundFiles;
    
    if (!PlatformFile->DirectoryExists(*DirectoryPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("디렉토리가 존재하지 않습니다: %s"), *DirectoryPath);
        return FoundFiles;
    }

    for (const FString& Pattern : FilePatterns)
    {
        TArray<FString> PatternFiles;
        PlatformFile->FindFilesRecursively(PatternFiles, *DirectoryPath, *Pattern);
        FoundFiles.Append(PatternFiles);
    }

    return FoundFiles;
}

bool UFileManager::CreateDirectory(const FString& DirectoryPath)
{
    return PlatformFile->CreateDirectoryTree(*DirectoryPath);
}

bool UFileManager::DirectoryExists(const FString& DirectoryPath)
{
    return PlatformFile->DirectoryExists(*DirectoryPath);
}

TArray<FString> UFileManager::GetSubDirectories(const FString& DirectoryPath)
{
    TArray<FString> SubDirs;
    
    if (!PlatformFile->DirectoryExists(*DirectoryPath))
    {
        return SubDirs;
    }

    TArray<FString> AllItems;
    PlatformFile->FindFiles(AllItems, *DirectoryPath, TEXT("*"));
    
    for (const FString& Item : AllItems)
    {
        if (PlatformFile->DirectoryExists(*Item))
        {
            SubDirs.Add(Item);
        }
    }

    return SubDirs;
}

bool UFileManager::FileExists(const FString& FilePath)
{
    return PlatformFile->FileExists(*FilePath);
}

int64 UFileManager::GetFileSize(const FString& FilePath)
{
    if (!PlatformFile->FileExists(*FilePath))
    {
        return -1;
    }
    
    return PlatformFile->FileSize(*FilePath);
}

FString UFileManager::GetFileName(const FString& FilePath)
{
    return FPaths::GetBaseFilename(FilePath);
}

FString UFileManager::GetRelativePath(const FString& FullPath, const FString& BasePath)
{
    FString RelativePath = FullPath;
    FPaths::MakePathRelativeTo(RelativePath, *BasePath);
    return RelativePath;
}

bool UFileManager::CopyFile(const FString& SourcePath, const FString& DestPath)
{
    if (!PlatformFile->FileExists(*SourcePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("소스 파일이 존재하지 않습니다: %s"), *SourcePath);
        return false;
    }

    FString FileContent;
    if (FFileHelper::LoadFileToString(FileContent, *SourcePath))
    {
        return FFileHelper::SaveStringToFile(FileContent, *DestPath);
    }
    
    return false;
}

bool UFileManager::MoveFile(const FString& SourcePath, const FString& DestPath)
{
    if (!PlatformFile->FileExists(*SourcePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("소스 파일이 존재하지 않습니다: %s"), *SourcePath);
        return false;
    }

    return PlatformFile->MoveFile(*DestPath, *SourcePath);
}

TArray<FString> UFileManager::GetDefaultFilePatterns()
{
    return {
        TEXT("*.txt"),
        TEXT("*.uasset"),
        TEXT("*.umap"),
        TEXT("*.ini"),
        TEXT("*.bat"),
        TEXT("*.md"),
        TEXT("*.*")
    };
} 