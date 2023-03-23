#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>


int main(int argc, char** argv)
{
    std::cout << "pkodev.tool.objreader v1.0\r\n\r\n";

    if (argc != 2) {
        std::cout << "Usage:\r\n\tpkodev.tool.objreader [path to .obj file]\r\n";
        return 1;
    }

    std::ifstream file(argv[1], std::ios::binary);
    if (file.is_open() == false) {
        std::cout << "Failed to open file \"" << argv[1] << "\"!\r\n";
        return 2;
    }

    std::cout << "Working...\r\n" << std::endl;

    char szTitle[16]{ 0x00 };
    file.read(szTitle, sizeof(szTitle));
    if (std::string(szTitle) != "HF Object File!") {
        std::cout << "Wrong .obj file format!\r\n";
        return 3;
    }

    unsigned int nVersion = 0u;
    file.read(reinterpret_cast<char*>(&nVersion), sizeof(nVersion));
    if (nVersion != 600u) {
        std::cout << "Unsupported .obj file version!\r\n";
        return 4;
    }

    unsigned int nFileSize = 0u;
    file.read(reinterpret_cast<char*>(&nFileSize), sizeof(nFileSize));

    std::cout << argv[1] << ":\r\n\tTitle: " << szTitle << "\r\n\tVersion: " << nVersion
        << "\r\n\tFile size: " << nFileSize << " bytes\r\n" << std::endl;

    unsigned int nSectionCntX(0u), nSectionCntY(0u), nSectionWidth(0u),
        nSectionHeight(0u), nSectionObjNum(0u);

    file.read(reinterpret_cast<char*>(&nSectionCntX), sizeof(nSectionCntX));
    file.read(reinterpret_cast<char*>(&nSectionCntY), sizeof(nSectionCntY));
    file.read(reinterpret_cast<char*>(&nSectionWidth), sizeof(nSectionWidth));
    file.read(reinterpret_cast<char*>(&nSectionHeight), sizeof(nSectionHeight));
    file.read(reinterpret_cast<char*>(&nSectionObjNum), sizeof(nSectionObjNum));
    
    const unsigned int nSectionCnt = nSectionCntX * nSectionCntY;

    std::cout << "Map information:"
        << "\r\n\tSections count X: "       << nSectionCntX
        << "\r\n\tSections count Y: "       << nSectionCntY
        << "\r\n\tSection width: "          << nSectionWidth
        << "\r\n\tSection height: "         << nSectionHeight
        << "\r\n\tMax objects in section: " << nSectionObjNum 
        << "\r\n" << std::endl;

    struct SSceneObjInfo final {
        int       nTypeID;
        int       nX;
        int  	  nY;
        short int nHeightOff;
        short int nYawAngle;
        int       nScale;
    };

    std::unordered_map<unsigned int, std::vector<SSceneObjInfo>> mSections;

    std::cout << "Reading sections...\r\n" << std::endl;

    unsigned int nObjInfoPos(0u), nObjNum(0), nTotalObjects(0u);
    for (unsigned int i = 0u; i < nSectionCnt; ++i)
    {
        file.read(reinterpret_cast<char*>(&nObjInfoPos), sizeof(nObjInfoPos));
        file.read(reinterpret_cast<char*>(&nObjNum), sizeof(nObjNum));

        if (nObjNum > 0u)
        {
            const std::streampos nCurOffset = file.tellg();
            file.seekg(static_cast<std::streampos>(nObjInfoPos), file.beg);

            std::vector<SSceneObjInfo> aObjects;
            aObjects.reserve(nObjNum);

            for (unsigned int j = 0u; j < nObjNum; ++j)
            {
                SSceneObjInfo Tmp;

                file.read(reinterpret_cast<char*>(&Tmp.nTypeID), sizeof(Tmp.nTypeID));
                file.read(reinterpret_cast<char*>(&Tmp.nX), sizeof(Tmp.nX));
                file.read(reinterpret_cast<char*>(&Tmp.nY), sizeof(Tmp.nY));
                file.read(reinterpret_cast<char*>(&Tmp.nHeightOff), sizeof(Tmp.nHeightOff));
                file.read(reinterpret_cast<char*>(&Tmp.nYawAngle), sizeof(Tmp.nYawAngle));
                file.read(reinterpret_cast<char*>(&Tmp.nScale), sizeof(Tmp.nScale));

                aObjects.push_back(Tmp);
            }

            mSections[i] = std::move(aObjects);

            nTotalObjects += nObjNum;

            file.seekg(nCurOffset, file.beg);
        }
    }

    std::cout << "Total sections read: " << mSections.size() << "\r\n"
        << "Total objects read: " << nTotalObjects << "\r\n" << std::endl;

    auto GetObjectType = [](int nTypeID) -> unsigned short int {
        return static_cast<unsigned short int>(nTypeID >> (sizeof(short int) * 8 - 2));
    };

    auto GetObjectID = [](int nTypeID) -> unsigned short int {
        return static_cast<unsigned short int>(~(3 << (sizeof(short) * 8 - 2)) & nTypeID);
    };

    unsigned int nCounter = 0u;

    std::cout << "#\tID\tX\tY\tAngle\tHeight\tScale\r\n\r\n";

    for (const auto& [nSectionIdx, aObjects] : mSections)
    {
        const int nSectionX = nSectionIdx % nSectionCntX * nSectionWidth * 100u;
        const int nSectionY = nSectionIdx / nSectionCntY * nSectionHeight * 100u;

        for (const auto& Object : aObjects)
        {
            const int nObjectX = (Object.nX + nSectionX) / 100u;
            const int nObjectY = (Object.nY + nSectionY) / 100u;

            std::cout << ++nCounter << '.'
                << "\t" << GetObjectID(Object.nTypeID)
                << "\t" << nObjectX
                << "\t" << nObjectY
                << "\t" << Object.nYawAngle
                << "\t" << Object.nHeightOff
                << "\t" << Object.nScale
                << "\r\n";
        }
    }

    return 0;
}
