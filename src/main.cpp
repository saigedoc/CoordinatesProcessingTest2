#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <regex>


using boost::asio::ip::tcp;
using json = nlohmann::json;
namespace fs = std::filesystem;


int main(){
    SetConsoleCP(65001);// установка кодовой страницы win-cp 1251 в поток ввода
    SetConsoleOutputCP(65001); // установка кодовой страницы win-cp 1251 в поток вывода
    printf("ТЕСТ\n");
    printf("g-1\n");
    std::ifstream coords("coordinates.txt");
    std::vector<std::string> coords_list;
    std::string line;
    while (std::getline(coords, line)){
        int double_space = line.find("  ");
        if (double_space != -1){
            line.erase(double_space, line.length() - 1);
        }
        //std::cout << "cal " << line << std::endl;
        line = std::regex_replace(line, std::regex(", "), " ");
        coords_list.push_back(line);
    }

    std::string path = "texts";
    std::vector<std::string> files_path;
    std::string p;
    json output = json::array();
    int file_num = 0;
    for (const auto & entry : fs::directory_iterator(path)){
        //std::cout << entry.path().string() << std::endl;
        //p = entry.path().string();
        /*
        1. найти совпадения.
        2. Определить тип свопадающх координат
        3. Название если есть
        4. Формат (Decimal Degrees, DD 0.1 -(s)0.1 / Degrees and Decimal Minutes, DDM  -(a)0-1 (b)00-10 / DMM compact 0100(a) / DMS/DDM compact 01000.00(a))

        [
            {
            "coordinates":[
                {
                "group": 1,
                "coords":[
                    {
                    "coord": 12.2112 -32.434,
                    "sentence": some text,
                    "type": part of poligon / part of line / point,
                    },
                    {
                    "coord": 12.2112 -32.434,
                    "in sentence": some text,
                    "type": part of poligon / part of line / point,
                    "format": DD
                    }
                ]
                }
                    
                ] 
            }
        ]

        */
        //printf("%s\n", entry.path().string().c_str());
        //printf("g0\n");
        output.push_back({{"coordinates", json::array({{{"group", 1}, {"coords",   json::array()}}})}});
        std::ifstream text(entry.path().string());
        //printf("g\n");
        
        while (std::getline(text, line)){
            for (int i = 0; i < coords_list.size(); ++i){
                std::string coord = coords_list[i];
                std::string c = coord.substr(coord.find(" ") + 1, coord.length() - coord.find(" ") + 1);
                std::string c0 = c.substr(0, c.find(" "));
                std::string c1 = c.substr(c.find(" ") + 1, c.length() - c.find(" ") - 1);
                size_t lf = line.find(c0);
                size_t lf1 = line.find(c1);
                
                if ((lf == -1) || (lf1 == -1)){
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
                if ((c.find(".") != -1) && (c.find("°") != -1) && (c.find("'") != -1)){
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
                //printf("g1\n");
                //std::cout << "t1" << std::endl;
                if (output[file_num]["coordinates"].back()["group"] != std::atoi(&coord[0])){ //проверка на группу, если новая, определяем тип прошлой.
                    json c_list = output[file_num]["coordinates"].back()["coords"];
                    if (c_list.size() == 1){
                        //printf("g3\n");
                        output[file_num]["coordinates"].back()["coords"][0]["type"] = "point";
                    }
                    else {
                        if (std::adjacent_find(c_list.begin(), c_list.end()) != c_list.end()){
                            for (int i = 0; i < c_list.size(); ++i){
                                //printf("g4\n");
                                output[file_num]["coordinates"].back()["coords"][i]["type"] = "part of poligon";
                            }
                            
                        }
                        else {
                            for (int i = 0; i < c_list.size(); ++i){
                                //printf("g5\n");
                                output[file_num]["coordinates"].back()["coords"][i]["type"] = "part of line";
                            }
                        }
                    }
                    //printf("g6\n");
                    //std::cout << "coord" << coord << std::endl;
                    output[file_num]["coordinates"].push_back(json::array({{{"group", std::atoi(&coord[0])}, {"coords",   json::array()}}})); //новая группа
                    //std::cout << output << std::endl;
                }
                //printf("g7\n");
                //std::cout << output << std::endl;
                //std::cout << "gwgw    "<< output[file_num]["coordinates"].back() << std::endl;
                
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
    
    std::cout << output << std::endl;
    std::cout << "END" << std::endl;


    boost::asio::io_context io_context;
    tcp::acceptor server(io_context, tcp::endpoint(tcp::v4(), 8080));

    for (;;) {
        tcp::socket socket(io_context);
        server.accept(socket);

        std::string request(1024, 0);
        boost::system::error_code ec;
        size_t length = socket.read_some(boost::asio::buffer(request), ec);
        if (ec) continue;

        std::string json = R"({"message": "Hello, World!"})";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n" + json;

        boost::asio::write(socket, boost::asio::buffer(response));
    }
    return 0;
}