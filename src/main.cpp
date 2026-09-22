/*
 * baregear - A programming language compiler
 * Copyright (C) 2026 Abdullah Al Nahian Raiyan <abdullahal3829@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCryptographicHash>
#include <QDebug>
#include <fstream>
#include <definations.h>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QUrl>
#include <QProcess>
#include <QEventLoop>
#include <zlib.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <seccomp.h>

#include <lexer.h>
#include <parser.h>
#include <preprocessor.h>
#include <transpiler.h>

// Not needed http or https value of BUG_REPORT_URL
#define BUG_REPORT_URL              "www.example.com/bugs"
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

std::stringstream stream;
std::vector<std::string> sourceLines;
bool errorHappens = false;
QCommandLineParser qparser;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-narrowing"
namespace {
    #include <success_audio.h>
    #include <failure_audio.h>

    ma_engine engine;
    bool engine_inited = false;

    void play_audio(const unsigned char* data, size_t size) {
        if (!engine_inited) {
            ma_engine_config cfg = ma_engine_config_init();
            if (ma_engine_init(&cfg, &engine) != MA_SUCCESS)
                return;
            engine_inited = true;
        }

        ma_decoder decoder;
        ma_decoder_config dcfg = ma_decoder_config_init(ma_format_unknown, 0, 0);
        if (ma_decoder_init_memory(data, size, &dcfg, &decoder) != MA_SUCCESS)
            return;

        ma_sound sound;
        if (ma_sound_init_from_data_source(&engine, &decoder, 0, NULL, &sound) != MA_SUCCESS) {
            ma_decoder_uninit(&decoder);
            return;
        }

        ma_sound_start(&sound);

        while (ma_sound_is_playing(&sound))
            ma_sleep(50);

        ma_sound_uninit(&sound);
        ma_decoder_uninit(&decoder);
    }
}
#pragma clang diagnostic pop

void success() { 
    play_audio(success_mp3, success_mp3_len); 
}

void failure() { 
    play_audio(failure_mp3, failure_mp3_len); 
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QString("Baregear Compiler v%1\nCopyright (c) 2026 Abdullah Al Nahian Raiyan <abdullahal3829@gmail.com>").arg(APP_VERSION));

    QCommandLineOption helpOption(QStringList{"h", "help"}, "Displays help on commandline options.");
    qparser.addOption(helpOption);

    QCommandLineOption inputFileOption(QStringList{"o", "output"}, "Output file.", "output");
    qparser.addOption(inputFileOption);

    QCommandLineOption modeOption("mode", "Compilation Mode", "mode");
    qparser.addOption(modeOption);

    QCommandLineOption defineOption(QStringList{"d", "define"}, "Define a macro (NAME or NAME=VALUE).", "define");
    qparser.addOption(defineOption);

    qparser.addPositionalArgument("source", "Source file to compile.", "source");

    qparser.process(app);

    const QStringList positionalArgs = qparser.positionalArguments();
    if (qparser.isSet(helpOption) || positionalArgs.isEmpty()) {
        QString help = qparser.helpText();
        int firstNewline = help.indexOf('\n');
        if (firstNewline != -1) {
            std::cout << QCoreApplication::applicationName().toStdString() << std::endl;
            std::cout << std::endl;
            int bracketPos = help.indexOf('[');
            std::cout << "Usage: baregear " << help.mid(bracketPos, firstNewline - bracketPos).toStdString() << std::endl;
            std::cout << help.mid(firstNewline + 1).toStdString();
        } else
            std::cout << help.toStdString();
        return positionalArgs.isEmpty() ? 1 : 0;
    }

    QString sourceFile = positionalArgs.first();
    std::ifstream file(sourceFile.toStdString());
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file of " << sourceFile.toStdString() << std::endl;
        failure();
        return 1;
    }

    std::string sourceCode((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
    file.close();
    stream << sourceCode;
    
    // Store source code lines for error reporting
    std::istringstream iss(sourceCode);
    std::string line;
    while (std::getline(iss, line))
        sourceLines.push_back(line);

    Lexer lex = Lexer(sourceCode);
    std::vector<TOKEN> toks = lex.lex();
    Parser parser = Parser(toks);
    std::vector<AST*> ast = parser.parse();
    Preprocessor pp = Preprocessor(ast);
    CompilerMetadata meta;
    const QStringList defines = qparser.values(defineOption);
    for (const QString& def : defines) {
        std::string d = def.toStdString();
        size_t eq = d.find('=');
        if (eq != std::string::npos)
            meta.defines[d.substr(0, eq)] = d.substr(eq + 1);
        else
            meta.defines[d] = "";
    }
    Transpiler trns = Transpiler(pp.preprocess(&meta));
    std::cout << trns.transpile() << std::endl;

    return 0;
}

std::string getLine(int index) {
    if (index >= 1 && index <= sourceLines.size())
        return sourceLines[index - 1];
    return "";
}

void error(std::string message, unsigned int row, unsigned int col) {
    errorHappens = true;
    printNotice(row, col, 196, message);
}

void warn(std::string message, unsigned int row, unsigned int col) {
    printNotice(row, col, 226, message);
}

extern "C" {
    void bugDetected(const char* message) {
        // Calculate CRC32 from disk
        std::ifstream file("/proc/self/exe", std::ios::binary);
        std::string diskCRC32;
        
        if (file.is_open()) {
            uLong crc = crc32(0L, Z_NULL, 0);
            std::vector<char> buffer(4096);
            
            while (file.read(buffer.data(), buffer.size()))
                crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), file.gcount());
            
            if (file.gcount() > 0)
                crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), file.gcount());
            file.close();
            
            // Convert CRC32 to hex string
            std::stringstream ss;
            ss << std::hex << std::setw(8) << std::setfill('0') << crc;
            diskCRC32 = ss.str();
        }
        
        // Calculate CRC32 from RAM (in-memory executable)
        std::string exePath = QCoreApplication::applicationFilePath().toStdString();
        std::ifstream ramFile(exePath, std::ios::binary);
        std::string ramCRC32;
        
        if (ramFile.is_open()) {
            uLong crc = crc32(0L, Z_NULL, 0);
            std::vector<char> buffer(4096);
            
            while (ramFile.read(buffer.data(), buffer.size()))
                crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), ramFile.gcount());
            
            if (ramFile.gcount() > 0)
                crc = crc32(crc, reinterpret_cast<const Bytef*>(buffer.data()), ramFile.gcount());
            ramFile.close();
            
            // Convert CRC32 to hex string
            std::stringstream ss;
            ss << std::hex << std::setw(8) << std::setfill('0') << crc;
            ramCRC32 = ss.str();
        }
        
        // Compare checksums
        if (diskCRC32 != ramCRC32) {
            // Checksums differ - restart application
            qCritical() << "Software Is Corrupted on RAM. Restarting application...";
            QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
            QCoreApplication::exit(0);
            return;
        }
        
        // Check if in release mode
        #ifdef QT_DEBUG
            // Debug mode - display bug box
            std::cout << "========================================" << std::endl;
            std::cout << "|           BUG DETECTED               |" << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << "| Message: " << message << std::endl;
            std::cout << "========================================" << std::endl;
        #elifdef BUG_REPORT_URL
            // Release mode - report bug via HTTP
            QNetworkAccessManager manager;
            QNetworkRequest request(QUrl(QString("http://%1").arg(BUG_REPORT_URL)));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            
            QString url;
            if (!diskCRC32.empty())
                url = QString("http://%1?title=%2&crc32=%3")
                            .arg(BUG_REPORT_URL)
                            .arg(QString::fromStdString(message))
                            .arg(QString::fromStdString(diskCRC32));
            else
                url = QString("http://%1?title=%2")
                            .arg(BUG_REPORT_URL)
                            .arg(QString::fromStdString(message));

            request.setUrl(QUrl(url));
            
            QNetworkReply* reply = manager.get(request);
            
            QObject::connect(reply, &QNetworkReply::finished, [reply, message]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray response = reply->readAll();
                    QJsonDocument doc = QJsonDocument::fromJson(response);
                    
                    if (doc.isObject()) {
                        QJsonObject obj = doc.object();
                        int requestType = obj["request_type"].toInt();
                        QString result = obj["result"].toString();
                        
                        if (requestType == 400 && result == "ERR_SOFTWARE_CORRUPTED") {
                            // Print red error and close
                            std::cout << "\033[31m" << "ERROR: " <<
                                            "Baregear Compiler Is Corrupted Please Reinstall." << "\033[0m" << std::endl;
                            QCoreApplication::exit(1);
                        }
                    }
                }
                reply->deleteLater();
            });
            
            // Wait for async request to complete
            QEventLoop loop;
            QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
        #endif
    }
}