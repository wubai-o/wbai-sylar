#include"mymysql.h"

namespace wubai {

static std::string data;

bool Mysql::connect() {
    if(!mysql_real_connect(&m_mysql, m_host, m_user, m_password, m_db, m_port, m_uinx_socket, m_client_flag)) {
        std::cout << "connect fail" << std::endl;
        return false;
    }
    return true;

}

void Mysql::addData(std::string str) {
    std::stringstream ss;
    std::string tmp;
    std::vector<std::string> svec;
    ss << str;
    while(getline(ss, tmp, ',')) {
        if(tmp.size() == 0) {
            break;
        }
        svec.push_back(tmp);
    }
    //std::string sql = "INSERT INTO `car_cycle_data`(id,encoder_value_right,encoder_value_left,acceleration_x,acceleration_y,acceleration_z,gyroscope_x,gyroscope_y,gyroscope_z,magnetometer_x,magnetometer_y,magnetometer_z,electromagnetic_right,electromagnetic_center,electromagnetic_left,off_center,steering_gear_control,motor_control_left,motor_control_right,P_value,I_value,D_value) VALUES('" + svec[0] + "','" + svec[1] + "','"+ svec[2] + "','"+ svec[3] + "','"+ svec[4] + "','"+ svec[5] + "','"+ svec[6] + "','"+ svec[7] + "','"+ svec[8] + "','"+ svec[9] + "','" + svec[10] + "','"+ svec[11] + "','"+ svec[12] + "','"+ svec[13] + "','"+ svec[14] + "','"+ svec[15] + "',"+ svec[16] + ","+ svec[17] + ",'"+ svec[18] + "','"+ svec[19] + "','" + svec[20] + "','" + svec[21] + "');";
    std::string sql = "INSERT INTO `car_cycle_data`(id,encoder_value_right,encoder_value_left,acceleration_x,acceleration_y,acceleration_z,gyroscope_x,gyroscope_y,gyroscope_z,magnetometer_x,magnetometer_y,magnetometer_z,electromagnetic_right,electromagnetic_center,electromagnetic_left,off_center,steering_gear_control,motor_control_left,motor_control_right,P_value,I_value,D_value) VALUES(" + svec[0] + "," + svec[1] + ","+ svec[2] + ","+ svec[3] + ","+ svec[4] + ","+ svec[5] + ","+ svec[6] + ","+ svec[7] + ","+ svec[8] + ","+ svec[9] + "," + svec[10] + ","+ svec[11] + ","+ svec[12] + ","+ svec[13] + ","+ svec[14] + "," + svec[15] + ","+ svec[16] + ","+ svec[17] + ","+ svec[18] + ","+ svec[19] + "," + svec[20] + "," + svec[21] + ");";
    mysql_query(&m_mysql, sql.c_str());
}

void Mysql::close() {
    mysql_close(&m_mysql);
}

}