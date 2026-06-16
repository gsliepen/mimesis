// load and parse a eml file, which have attachments

#include "mimesis.hpp"
#include <iostream>
#include <fstream>
#include <vector>

#ifdef _WIN32

using TString = std::wstring;
#define TOUT std::wcout
#define TEXT(xxx) L##xxx

#else

using TString = std::string;
#define TOUT std::cout
#define TEXT(xxx) xxx

#endif

void Test_ParseEmlFile(TString path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cout << "file not found" << std::endl;
        return;
    }
    Mimesis::Message msg;
    try
    {
        msg.load(file);
    }
    catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
        return;
    }
    std::vector<const Mimesis::Part *> files;
    files = msg.get_attachments();
    std::cout << "Attachments: " << files.size() << std::endl;
    std::cout << "From:  " << msg.get_header_value("From");
    TOUT << TEXT("From:(decodeed)  ") << Mimesis::decode_header(msg.get_header_value("From")) << std::endl;
    std::cout << "To:  " << msg.get_header_value("To");
    TOUT << TEXT("To:(decodeed)  ") << Mimesis::decode_header(msg.get_header_value("To")) << std::endl;
    
    for (size_t i = 0; i < files.size(); ++i)
    {
        const Mimesis::Part *part = files[i];

        auto headers = part->get_headers();
        for (auto it : headers)
        {
            std::cout << "header:" << it.first << "  " << it.second << std::endl;
        }

        // 获取文件名（优先从 Content-Disposition 获取，如果没有则从 Content-Type 获取）
        std::string filename = part->get_header_parameter("Content-Disposition", "filename");
        if (filename.empty())
        {
            filename = part->get_header_parameter("Content-Type", "name");
        }
        // 解码 RFC 编码的文件名
        // decode file name
        TString decoded_filename = Mimesis::decode_header(filename);
        // 获取附件内容
        std::string content = part->get_body(); //  get attachment content

        std::cout << "\n attachment " << (i + 1) << ":" << std::endl;
        std::cout << "  file name: " << (filename.empty() ? "(no named)" : filename) << std::endl;
        TOUT << TEXT("  Decoded file name: ") << (decoded_filename.empty() ? TEXT("(no named)") : decoded_filename) << std::endl;
        std::cout << "  Content Size: " << content.size() << " Bytes" << std::endl;
        std::cout << "  MIME Type: " << part->get_header("Content-Type") << std::endl;

        // 如果需要查看内容（注意：二进制文件可能显示乱码）
        // If you need to view the content (Note: Binary files may display unreadable characters)
        if (!content.empty() && content.size() < 200)
        {
            std::cout << "  content: " << content << std::endl;
        }

        // save attachment to file
        if (!decoded_filename.empty() && !content.empty())
        {
            std::ofstream outfile(decoded_filename, std::ios::binary);
            if (outfile.is_open())
            {
                outfile.write(content.data(), content.size());
                outfile.close();
                TOUT << TEXT("  saved: ") << decoded_filename << std::endl;
            }
            else
            {
                TOUT << TEXT("  cannot save: ") << decoded_filename << std::endl;
            }
        }
    }

    file.close();
    return;
}

int main()
{
    std::cout << "Hello, from mimesis!" << std::endl;
#ifdef _WIN32
    std::wcout.imbue(std::locale("chs")); // set your language   windows  chs/us
#else
    std::wcout.imbue(std::locale("zh_CN.UTF-8")); // set your language  linux  
#endif

    Test_ParseEmlFile(TEXT("d:\\test.eml")); //// input your file path


    return 0;
}