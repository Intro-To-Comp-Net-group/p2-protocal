//
// Created by Hanyi Wang on 3/6/20.
//

#ifndef PROJECT2_UTILS_H
#define PROJECT2_UTILS_H

#include <fstream>

using namespace std;

bool check_file_existence(const string &file_path) {
    ifstream file(file_path);
    if (!file) return false;
    else {
        file.close();
        return true;
    }
}


#endif //PROJECT2_UTILS_H
