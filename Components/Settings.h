#pragma once
#ifndef __SETTINGS_H
    #define __SETTINGS_H
    #include <stdio.h>
    #include <string>

    class Settings{
        private:
            const std::string SETTINGS_FILE_PATH = "./Components/Settings.txt";
            std::string PATH_PREFIX              = "./__dist__/";
            std::string PROJECTNAME              = "NULL";
            std::string FOLDERNAME_DEBUG         = "debug/";
            std::string FOLDERNAME_TMP           = "tmp/";
            // overwrite the setting
            void _Overwrite(){
                std::fstream file_in(Settings::SETTINGS_FILE_PATH, std::fstream::out);
                file_in << this->PATH_PREFIX << '\n';
                file_in << this->PROJECTNAME << '\n';
                file_in << this->FOLDERNAME_DEBUG << '\n';
                file_in << this->FOLDERNAME_TMP << '\n';
            }

        public:
            const std::string TRACK_FILE_YANS_WIFI_PHY                  = "track_yas-wifi-phy.csv";
            const std::string TRACK_FILE_YANS_WIFI_PHY_DATA             = "track_yas-wifi-phy_data.csv";
            const std::string TRACK_FILE_YANS_WIFI_PHY_DATA_BEACON      = "track_yas-wifi-phy_data-beacon.csv";

            /**
             * constructor 
             */
            Settings(){
                std::ifstream file_out(Settings::SETTINGS_FILE_PATH);
                if (file_out.is_open()){
                    file_out >> this->PATH_PREFIX;
                    file_out >> this->PROJECTNAME;
                    file_out >> this->FOLDERNAME_DEBUG;
                    file_out >> this->FOLDERNAME_TMP;
                }else{
                    _Overwrite();
                }
            }

            /**
             * Set the Project Name
             * @strProjectName: the name of the project (supporting const char *)
             */
            void SetProjectName(const char * strProjectName){
                if (strProjectName){
                    this->PROJECTNAME = strProjectName;     // set
                    if(this->PROJECTNAME.back() != '/'){
                        this->PROJECTNAME.append("/");
                    }
                    _Overwrite();                           // overwritten setings
                }
            }
            /**
             * return this project folder path
             */
            std::string PathProject(){
                std::string projectName = "";
                if (this->PROJECTNAME != "NULL"){
                    projectName = this->PROJECTNAME; 
                }
                return this->PATH_PREFIX + projectName;
            }
            /**
             * return project-debug folder path
             */
            std::string PathProjectDebug(){
                std::string projectName = "";
                if (this->PROJECTNAME != "NULL"){
                    projectName = this->PROJECTNAME; 
                }
                return this->PATH_PREFIX + projectName + this->FOLDERNAME_DEBUG;
            }
            /**
             * return project-tmp folder path
             */
            std::string PathProjectTmp(){
                std::string projectName = "";
                if (this->PROJECTNAME != "NULL"){
                    projectName = this->PROJECTNAME; 
                }
                return this->PATH_PREFIX + projectName + this->FOLDERNAME_TMP;
            }
    };
#endif