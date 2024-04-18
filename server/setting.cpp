#include "setting.h"

#include <fstream>
#include <algorithm>
#include <iostream>

Setting::Setting(const string file)
{
    readSettingFile(file);
}

void Setting::splitKeyvalue(const string &s, string &key, string &value)
{
    int pos = s.find('=');
    key = s.substr(0, pos);
    value = s.substr(pos + 1);
}

void Setting::readSettingFile(const string &filename)
{
    ifstream fin(filename);
    string line;
    while (getline(fin, line)) {
        string key, value;
        // only split lines with '='
        if (line.find('=') == string::npos) {
            continue;
        }
        splitKeyvalue(line, key, value);
        settings[key] = value;
    }
}

string Setting::getStringValue(const string &key, const string &def)
{
    if (settings.find(key) == settings.end()) {
        return def;
    }
    else {
        return settings.at(key);
    }
}

int Setting::getIntValue(const string &key, int def)
{
    if (settings.find(key) == settings.end()) {
        return def;
    }
    else {
        return stoi(settings.at(key));
    }
}

bool Setting::getBoolValue(const string &key, bool def)
{
    if (settings.find(key) == settings.end()) {
        return def;
    }
    else {
        string value = settings.at(key);
        transform(value.begin(), value.end(), value.begin(), ::tolower);
        return value == "true";
    }
}

float Setting::getFloatValue(const string &key, float def)
{
    if (settings.find(key) == settings.end()) {
        return def;
    }
    else {
        return stof(settings.at(key));
    }
}
