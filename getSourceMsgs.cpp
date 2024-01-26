#include <dirent.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>

using namespace std;  
//#define FILE_SYS std::filesystem
namespace FILE_SYS = std::filesystem;

// Local forward decls
void doDisplayUseage();

void extractFromFilesInFolder(const string&, const string&);
void extractFromThisFile(const string&, const string&);

void writeThisMsgIdString(fstream* fileOutStream,
    const string& inputLine);
string writeNextMsgStrString(fstream* fileInStream,
    fstream* fileOutStream);

bool endsWith(const string& str, const string& postfix);

// Nice. Avoid creating 0 length files.
bool mThatOutputFileEmpty = false;

/** *********************************************************************
 ** lookFor -s string -t target -e endsWith.
 */
int main(int argc, char* argv[]) {
    // String, and guard the inputs.
    string msgString = "";
    string sourcePoFilesFolder = "";
    if (argc > 1) {
        msgString = string(argv[1]);
        if (argc > 2) {
            sourcePoFilesFolder = string(argv[2]);
        }
    }

    // Require a string to translate.
    if (msgString.empty()) {
        cout << "\033[33;1m\nNo MsgId String provided, halting.\033[0m"
            << endl;
        doDisplayUseage();
        return 0;
    }

    // Default to working directory.
    if (sourcePoFilesFolder.empty()) {
        cout << "\033[33;1m\nNo Gtk PO Source Folder provided, halting.\033[0m"
            << endl;
        doDisplayUseage();
        return 0;
    }

    // Ensure Gtk source exists, and is a directory.
    FILE_SYS::path thisFolder = FILE_SYS::canonical(FILE_SYS::current_path());
    if (!FILE_SYS::exists(sourcePoFilesFolder)) {
        cout << "\033[33;1mTarget doesn\'t exist, halting.\033[0m"
            << endl;
        return 0;
    }

    if (!FILE_SYS::is_directory(
        FILE_SYS::status(sourcePoFilesFolder))) {
        cout << "\033[33;1mTarget isn\'t a directory, halting.\033[0m"
            << endl;
        return 0;
    }

    // Gopher it!
    extractFromFilesInFolder(FILE_SYS::canonical(
        sourcePoFilesFolder), msgString);
}

/** *********************************************************************
 ** Display useage (All Supported Commands).
 **/
void doDisplayUseage() {
    cout << "\033[34;1m\nUseage: getGtkMsgs MSG_STRING "
        "[FROM_FOLDER]\033[0m" << endl << endl;
}

/** *********************************************************************
 ** Main loop, recurse through dirs, print matches.
 */
void extractFromFilesInFolder(const string& folder,
    const string& msgString) {

    DIR* dirFileHandle = opendir(folder.c_str());
    if (!dirFileHandle) {
        return;
    }

    struct dirent* dirFileEntry;
    while ((dirFileEntry = readdir(dirFileHandle)) != NULL) {
        string fileInFolder = dirFileEntry->d_name;

        // Ignore "movement" entries.
        if (endsWith(fileInFolder, "..") ||
            endsWith(fileInFolder, ".")) {
            continue;
        }
        // Ignore all but .po entries.
        if (!endsWith(fileInFolder, ".po")) {
            continue;
        }

        // Get full file name, adjust for root.
        fileInFolder = (folder == "/") ?
            folder + fileInFolder :
            folder + "/" + fileInFolder;
        extractFromThisFile(fileInFolder, msgString);
    }

    closedir(dirFileHandle);
}

/** *********************************************************************
 ** Helper method for each dir entry.
 */
void extractFromThisFile(const string& thisFile,
    const string& msgString) {

    // Ensure source file is open, and user has access.
    fstream thisInputFileStream;
    thisInputFileStream.open(thisFile, ios::in);
    if (!thisInputFileStream.is_open()) {
        cout << "\033[33;1mCan\'t extract from this file: \033[0m"
            << thisFile << endl;
        return;
    }

    // Ensure target file is open, and user has access.
    string thatOutputFile =
        thisFile.substr(thisFile.find_last_of("/") + 1);
    fstream thatOutputFileStream;
    thatOutputFileStream.open(thatOutputFile,
        ios::out | ios::binary);

    if (!thatOutputFileStream.is_open()) {
        thisInputFileStream.close();
        cout << "\033[33;1mCan\'t extract to that file: \033[0m"
            << thatOutputFile << endl;
        return;
    }

    // Will remove file if empty, rather than
    // leaving a -0- size file in folder.
    mThatOutputFileEmpty = true;

    // Tricky ...
    string inputLine;
    string repositionLine;

    while(!thisInputFileStream.eof()) {
        // Avoids recursion later.
        if (!repositionLine.empty()) {
            inputLine = repositionLine;
            repositionLine = "";
        } else {
            getline(thisInputFileStream, inputLine);
        }

        // Can pass back a string for reconsideration.
        if (inputLine.find("msgid") != string::npos &&
            inputLine.find('"' + msgString + '"') != string::npos) {
            repositionLine = writeNextMsgStrString(&thisInputFileStream,
                &thatOutputFileStream);
        }
    }

    // Close, and return results.
    thatOutputFileStream.close();
    if (mThatOutputFileEmpty) {
        remove(thatOutputFile.c_str());
    }

    thisInputFileStream.close();
    return;
}

/** *********************************************************************
 ** Helper method. Write next input line containing 'msgstr'.
 */
string writeNextMsgStrString(fstream* fileInStream,
    fstream* fileOutStream) {

    string inputLine;
    while(!fileInStream->eof()) {
        getline(*fileInStream, inputLine);

        if (inputLine.find("msgstr") != string::npos &&
            inputLine.find('"' + '"') == string::npos) {
            writeThisMsgIdString(fileOutStream, inputLine);
            return "";
        }

        if (inputLine.find("msgid") != string::npos) {
            return inputLine;
        }
    }

    return "";
}

/** *********************************************************************
 ** Helper method. Write entire input line containing 'msgid'.
 */
void writeThisMsgIdString(fstream* fileOutStream,
    const string& inputLine) {
    *fileOutStream << inputLine << endl;

    mThatOutputFileEmpty = false;
}

/** *********************************************************************
 ** Helper method.
 */
bool endsWith(const string& str, const string& postfix) {
    if (postfix.size() > str.size()) {
        return false;
    }

    return equal(str.begin() + str.size() - postfix.size(),
                 str.end(), postfix.begin());
}