#include "plane.hpp"
#include "general.hpp"
#include <csv2.hpp>
#define PI 3.1415926f
static void csv_read(){
      csv2::Reader<csv2::delimiter<','>, 
               csv2::quote_character<'"'>, 
               csv2::first_row_is_header<true>,
               csv2::trim_policy::trim_whitespace> csv;           
  if (csv.mmap("foo.csv")) {
    const auto header = csv.header();
    for (const auto row: csv) {
      for (const auto cell: row) {
        // Do something with cell value
        // std::string value;
        // cell.read_value(value);
      }
    }
  }
}
/*考虑两种情况有插值表格，没有插值表格，考虑函数重载*/
Plane::Plane(){

};
Plane::~Plane(){

}
// 从MuJoCo仿真器获取位姿、速度，并完成坐标系转换
void Plane::Air_power_step(){
    // -------------------------- 1. 处理空速为0的边界情况 --------------------------
    if(speed_air_world_mag_mps < 1e-6){
        P_pa = 0.0;
        CL = CD = CY = 0.0;
        Cl = Cm = Cn = 0.0;
        L = D = Y = 0.0;
        l = m = n = 0.0;
        return;
    }

    // -------------------------- 2. 计算动压 --------------------------
    P_pa = 0.5 * rho_kgpm3 * pow(speed_air_world_mag_mps, 2);

    // -------------------------- 3. 舵面角度转弧度（输入是°） --------------------------
    double surface_rad[4]; // 临时变量，不新增成员
    for(int i=0; i<4; i++){
        surface_rad[i] = surface_d[i]*180/PI;
    }

    // -------------------------- 4. 无量纲角速度（飞行力学标准操作，临时变量） --------------------------
    double omega_dimless[3]; // 仅函数内临时变量，不新增类成员
    for(int i=0; i<3; i++){
        // 公式：无量纲角速度 = (有量纲角速度 × 参考长度) / (2 × 空速模值)
        omega_dimless[i] = (gyro_body_radps[i] * L_ref) / (2 * speed_air_world_mag_mps);
    }

    // -------------------------- 5. 计算气动力系数（CL/CD/CY） --------------------------
    // 升力系数CL：攻角+俯仰角速度+舵面贡献（修正枚举索引为AIR_前缀）
    CL = C[AIR_AOA][AIR_L] * AOA +                    // 攻角贡献
         C[AIR_Q][AIR_L] * omega_dimless[1] +         // 俯仰角速度q（无量纲）
         C[AIR_FR][AIR_L] * surface_rad[0] +          // 前右舵面
         C[AIR_FL][AIR_L] * surface_rad[1] +          // 前左舵面
         C[AIR_BL][AIR_L] * surface_rad[2] +          // 后左舵面
         C[AIR_BR][AIR_L] * surface_rad[3];           // 后右舵面

    // 阻力系数CD：零升阻力+攻角二次项+舵面阻力
    CD = C[0][AIR_D] +                                // 零升阻力CD0（C[0][AIR_D]为零值系数）
         C[AIR_AOA][AIR_D] * pow(AOA, 2) +            // 诱导阻力（攻角平方）
         C[AIR_FR][AIR_D] * pow(surface_rad[0], 2) +  // 舵面阻力（二次项）
         C[AIR_FL][AIR_D] * pow(surface_rad[1], 2) +
         C[AIR_BL][AIR_D] * pow(surface_rad[2], 2) +
         C[AIR_BR][AIR_D] * pow(surface_rad[3], 2);

    // 侧滑力系数CY：侧滑角+滚转/偏航角速度+舵面贡献
    CY = C[AIR_SOA][AIR_Y] * SOA +                    // 侧滑角核心贡献
         C[AIR_P][AIR_Y] * omega_dimless[0] +         // 滚转角速度p（无量纲）
         C[AIR_R][AIR_Y] * omega_dimless[2] +         // 偏航角速度r（无量纲）
         C[AIR_FR][AIR_Y] * surface_rad[0] +
         C[AIR_FL][AIR_Y] * surface_rad[1] +
         C[AIR_BL][AIR_Y] * surface_rad[2] +
         C[AIR_BR][AIR_Y] * surface_rad[3];

    // -------------------------- 6. 计算气动力矩系数（Cl/Cm/Cn） --------------------------
    // 滚转力矩系数Cl：侧滑角+滚转/偏航角速度+舵面
    Cl = C[AIR_SOA][AIR_CL] * SOA +                   // 侧滑角
         C[AIR_P][AIR_CL] * omega_dimless[0] +        // 滚转角速度p（无量纲）
         C[AIR_R][AIR_CL] * omega_dimless[2] +        // 偏航角速度r（无量纲）
         C[AIR_FR][AIR_CL] * surface_rad[0] +
         C[AIR_FL][AIR_CL] * surface_rad[1] +
         C[AIR_BL][AIR_CL] * surface_rad[2] +
         C[AIR_BR][AIR_CL] * surface_rad[3];

    // 俯仰力矩系数Cm：攻角+俯仰角速度+舵面
    Cm = C[AIR_AOA][AIR_CM] * AOA +                   // 攻角
         C[AIR_Q][AIR_CM] * omega_dimless[1] +        // 俯仰角速度q（无量纲）
         C[AIR_FR][AIR_CM] * surface_rad[0] +
         C[AIR_FL][AIR_CM] * surface_rad[1] +
         C[AIR_BL][AIR_CM] * surface_rad[2] +
         C[AIR_BR][AIR_CM] * surface_rad[3];

    // 偏航力矩系数Cn：侧滑角+滚转/偏航角速度+舵面
    Cn = C[AIR_SOA][AIR_CN] * SOA +                   // 侧滑角
         C[AIR_P][AIR_CN] * omega_dimless[0] +        // 滚转角速度p（无量纲）
         C[AIR_R][AIR_CN] * omega_dimless[2] +        // 偏航角速度r（无量纲）
         C[AIR_FR][AIR_CN] * surface_rad[0] +
         C[AIR_FL][AIR_CN] * surface_rad[1] +
         C[AIR_BL][AIR_CN] * surface_rad[2] +
         C[AIR_BR][AIR_CN] * surface_rad[3];

    // -------------------------- 7. 计算气动力（N） --------------------------
    L = P_pa * S_ref * CL; // 升力
    D = P_pa * S_ref * CD; // 阻力
    Y = P_pa * S_ref * CY; // 侧滑力

    // 叠加风扇推力（沿机体系x轴，与阻力反向）
    D -= this->fan_power_n;

    // -------------------------- 8. 计算气动力矩（Nm） --------------------------
    l = P_pa * S_ref * L_ref * Cl; // 滚转力矩
    m = P_pa * S_ref * L_ref * Cm; // 俯仰力矩
    n = P_pa * S_ref * L_ref * Cn; // 偏航力矩
}
