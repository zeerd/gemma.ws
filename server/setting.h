#ifndef SETTING_H
#define SETTING_H

#include <map>
#include <string>

using namespace std;
namespace std {

class Setting {
 public:
  Setting(const string file);

  string getStringValue(const string &key, const string &def = "");
  int getIntValue(const string &key, int def = 0);
  bool getBoolValue(const string &key, bool def = false);
  float getFloatValue(const string &key, float def = 0.0);

 private:
  void splitKeyvalue(const string &s, string &key, string &value);
  void readSettingFile(const string &filename);

 private:
  map<string, string> settings;
};

}  // namespace std

#endif /* SETTING_H */
