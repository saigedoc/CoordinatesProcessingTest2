#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>
#include <regex>

using boost::asio::ip::tcp;
using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * @brief Основная функция, осуществляет создание сервера и обработку координат.
 *
 * Открывает файл с координатами сохраняет в вектор.
 * Открывает и обрабатывает все текстовые файлы в папке texts.
 * Все обработанные данные записываются в json, пример архитектуры будет ниже.
 * Создаёт сервер на tcp://localhost:8080 с помощью boost asio.
 * Отправляет полученный json на страницу.
 * 
 * Архитектура json:
 * [
    {
        "coordinates":[
            {
                "group": 1,
                "coords":[
                    {
                        "coord": "12.2112 -32.434",
                        "sentence": "some text with coord",
                        "type": ("part of poligon" / "part of line" / "point"),
                        "format": ("Decimal Degrees, DD" / "Degrees and Decimal Minutes, DDM" / "DMM compact" / "DMS/DDM compact")
                    },
                    {
                        "coord": "12.2112 -32.434",
                        "in sentence": "some text with coord",
                        "type": ("part of poligon" / "part of line" / "point"),
                        "format": ("Decimal Degrees, DD" / "Degrees and Decimal Minutes, DDM" / "DMM compact" / "DMS/DDM compact")
                    }
                ]
            }
                    
        ] 
    }
    ]
 */

int main(){
    std::ifstream coords("coordinates.txt");
    std::vector<std::string> coords_list;
    std::string line;
    while (std::getline(coords, line)){
        int double_space = line.find("  ");
        if (double_space != -1){
            line.erase(double_space, line.length() - 1);
        }
        line = std::regex_replace(line, std::regex(", "), " ");
        coords_list.push_back(line);
    }

    std::string path = "texts";
    std::vector<std::string> files_path;
    std::string p;
    json output = json::array();
    int file_num = 0;
    for (const auto & entry : fs::directory_iterator(path)){ //Перебор файлов в папке texts
        output.push_back({{"coordinates", json::array({{{"group", 1}, {"coords",   json::array()}}})}});
        std::ifstream text(entry.path().string());

        while (std::getline(text, line)){
            for (int i = 0; i < coords_list.size(); ++i){
                std::string coord = coords_list[i];
                std::string c = coord.substr(coord.find(" ") + 1, coord.length() - coord.find(" ") + 1);
                std::string c0 = c.substr(0, c.find(" "));
                std::string c1 = c.substr(c.find(" ") + 1, c.length() - c.find(" ") - 1);
                size_t lf = line.find(c0);
                size_t lf1 = line.find(c1);
                if ((lf == -1) || (lf1 == -1)){ //Если нет совпадений -> проверка следующих координат в строке
                    continue;
                }
                output.back()["coordinates"].back()["group"] = std::atoi(&coord[0]);
                size_t s_b = line.substr(0, lf).rfind(". ");
                size_t s_e = line.substr(lf1 + c1.length(), line.length() - (lf1 + c1.length())).find(".");
                std::string sentence = line.substr(s_b, lf1 + 1 + c1.length() - s_b + s_e);
                std::string name = sentence.substr(0, lf);
                std::string format;
                size_t points = sentence.find(":");
                if ((points != -1) && (std::count(name.begin(), name.end(), ' ') < 4)){
                    name = name.substr(0, points);
                }
                else {
                    name = "";
                }
                if ((c.find(".") == std::string::npos) && (c.find("°") == std::string::npos) && (c.find("'") == std::string::npos)){
                    size_t defis = c.substr(1, c.length() - 1).find("-");
                    if (std::isdigit(c[defis - 1])){
                        format = "Degrees and Decimal Minutes, DDM";
                    }
                    else if(c.length() > 11){
                        format = "Degrees and Decimal Minutes compact, DMM compact";
                    }
                    else {
                        format = "DMS/DDM compact";
                    }   
                }
                else {
                    format = "Decimal Degrees, DD";
                }
                if (output[file_num]["coordinates"].back()["group"] != std::atoi(&coord[0])){ //проверка на группу, если новая, определяем тип прошлой.
                    json c_list = output[file_num]["coordinates"].back()["coords"];
                    if (c_list.size() == 1){
                        output[file_num]["coordinates"].back()["coords"][0]["type"] = "point";
                    }
                    else {
                        if (std::adjacent_find(c_list.begin(), c_list.end()) != c_list.end()){
                            for (int i = 0; i < c_list.size(); ++i){
                                output[file_num]["coordinates"].back()["coords"][i]["type"] = "part of poligon";
                            }
                        }
                        else {
                            for (int i = 0; i < c_list.size(); ++i){
                                output[file_num]["coordinates"].back()["coords"][i]["type"] = "part of line";
                            }
                        }
                    }
                    output[file_num]["coordinates"].push_back(json::array({{{"group", std::atoi(&coord[0])}, {"coords",   json::array()}}})); //новая группа
                }
                output[file_num]["coordinates"].back()["coords"].push_back({{"coord", c}, {"sentence", sentence}, {"type", ""}, {"format", format}, {"name", name}});
            }
        }
        json c_list = output[file_num]["coordinates"].back()["coords"]; // определение типа последней группы
        if (c_list.size() == 1){
            output[file_num]["coordinates"].back()["coords"][0]["type"] = "point";
        }
        else {
            if (std::adjacent_find(c_list.begin(), c_list.end()) != c_list.end()){
                for (int i = 0; i < c_list.size(); ++i){
                    output[file_num]["coordinates"].back()["coords"][i]["type"] = "part of poligon";
                }
                            
            }
            else {
                for (int i = 0; i < c_list.size(); ++i){
                    output[file_num]["coordinates"].back()["coords"][i]["type"] = "part of line";
                }
            }
        }
        file_num ++;
    }

    boost::asio::io_context io_context;
    tcp::acceptor server(io_context, tcp::endpoint(tcp::v4(), 8080));

    for (;;) {
        tcp::socket socket(io_context);
        server.accept(socket);

        std::string request(1024, 0);
        boost::system::error_code ec;
        size_t length = socket.read_some(boost::asio::buffer(request), ec);
        if (ec) continue;

        std::string json_raw = output.dump();
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(json_raw.size()) + "\r\n\r\n" + json_raw;

        boost::asio::write(socket, boost::asio::buffer(response));
    }
    return 0;
}