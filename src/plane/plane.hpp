#ifndef AIR_POWER_HPP
#define AIR_POWER_HPP
#include "general.hpp"
#include "world.hpp"
#include <mujoco/mujoco.h>
class Plane{
public:
    /**/
    /*先验参数*/
    double rho_kgpm3=1.225;//大气密度，近似不变
    double g_mps2=9.81;//重力，基本不用
    double S_ref = 0.0; //参考面积 (Reference Area) - 例如：弹翼总面积
    double L_ref = 0.0; //参考长度 (Reference Length) - 例如：弹体长度
    double mass_kg , I_kgm2[3][3] ;
    double C[AIR_FROM_LAST][AIR_POWER_LAST];//飞行力学系数，巨大表格
    /*时变变量*/
    double Q[4];//姿态四元数
    double rot_body2world[3][3];//旋转矩阵

    double AOA=0,SOA=0;
    double gyro_body_radps[3]={0};
    double speed_world_mps[3]={0};//绝对速度
    double speed_air_world_mps[3]={0};//相对地面空速，如果没有风和上面相等
    double speed_air_body_mps[3]={0};//空速机体系分量
    double speed_air_world_mag_mps=0;//取模
    
    /*可操纵变量*/
    double surface_d[4]={0};//舵面偏转°
    double fan_power_n=0;//风扇动力

    /*计算结果*/
    double P_pa;//动压
    double CL,CD,CY;
    double Cl,Cm,Cn;
    double L,D,Y;
    double l,m,n;
    /**/
    Plane();
    ~Plane();
    void Position_get(World& world);
    void Air_power_step();
    void Air_table_read();
};
#endif
