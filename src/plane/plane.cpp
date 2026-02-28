#include "plane.hpp"
#include "general.hpp"
#include <csv2.hpp>
#define PI 3.1415926f
static void csv_read()
{
     csv2::Reader<csv2::delimiter<','>,
                  csv2::quote_character<'"'>,
                  csv2::first_row_is_header<true>,
                  csv2::trim_policy::trim_whitespace>
         csv;
     if (csv.mmap("foo.csv"))
     {
          const auto header = csv.header();
          for (const auto row : csv)
          {
               for (const auto cell : row)
               {
                    // Do something with cell value
                    // std::string value;
                    // cell.read_value(value);
               }
          }
     }
}
/*考虑两种情况有插值表格，没有插值表格，考虑函数重载*/
Plane::Plane(int bodyid = 0)
{
     this->bodyid = bodyid;
};
Plane::~Plane()
{
}
// 获取仿真器中的数据
void Plane::Position_get(World &world)
{
     if (!world.m || !world.d)
     {
          throw std::runtime_error("[MuJoCo Error] world.m / world.d is nullptr");
     }
     const int quat_offset = 3;
     Q[0] = world.d->qpos[quat_offset + 0]; // w
     Q[1] = world.d->qpos[quat_offset + 1]; // x
     Q[2] = world.d->qpos[quat_offset + 2]; // y
     Q[3] = world.d->qpos[quat_offset + 3]; // z
     mjtNum rot_1d[9];
     mju_quat2Mat(rot_1d, Q);
     for (int i = 0; i < 3; i++)
     {
          for (int j = 0; j < 3; j++)
          {
               rot_body2world[i][j] = rot_1d[i * 3 + j];
          }
     }
     mjtNum vel_result[6]; //[角速度x, 角速度y, 角速度z, 线速度x, 线速度y, 线速度z]
     // 对象类型=mjOBJ_BODY，对象ID=0，结果存vel_result，坐标系=机体系（flg_local=1）
     mj_objectVelocity(world.m, world.d, mjOBJ_BODY, bodyid, vel_result, 1);

     // 5. 赋值机体系角速度gyro_body_radps[3]（取vel_result前3个元素）
     gyro_body_radps[0] = vel_result[0];
     gyro_body_radps[1] = vel_result[1];
     gyro_body_radps[2] = vel_result[2];

     // 6. 重新调用mj_objectVelocity获取世界系线速度（flg_local=0）
     mj_objectVelocity(world.m, world.d, mjOBJ_BODY, bodyid, vel_result, 0);
     // 赋值绝对速度speed_world_mps[3]（取vel_result后3个元素）
     speed_world_mps[0] = vel_result[3];
     speed_world_mps[1] = vel_result[4];
     speed_world_mps[2] = vel_result[5];
     // 无风
     speed_air_world_mps[0] = speed_world_mps[0];
     speed_air_world_mps[1] = speed_world_mps[1];
     speed_air_world_mps[2] = speed_world_mps[2];

     // 世界系空速转机体系（旋转矩阵转置）
     for (int i = 0; i < 3; i++)
     {
          speed_air_body_mps[i] = 0.0;
          for (int j = 0; j < 3; j++)
          {
               speed_air_body_mps[i] += rot_body2world[j][i] * speed_air_world_mps[j];
          }
     }
     speed_air_world_mag_mps = sqrt(
         pow(speed_air_world_mps[0], 2) +
         pow(speed_air_world_mps[1], 2) +
         pow(speed_air_world_mps[2], 2));

     if (speed_air_world_mag_mps > 1e-6)
     {
          AOA = atan2(-speed_air_body_mps[2], speed_air_body_mps[0]);
          double soa_ratio = speed_air_body_mps[1] / speed_air_world_mag_mps;
          soa_ratio = std::clamp(soa_ratio, -1.0, 1.0);
          SOA = asin(soa_ratio);
     }
     else
     {
          AOA = 0.0;
          SOA = 0.0;
     }
}
void Plane::Air_power_step()
{
     if (speed_air_world_mag_mps < 1e-6)
     {
          P_pa = 0.0;
          CL = CD = CY = 0.0;
          Cl = Cm = Cn = 0.0;
          L = D = Y = 0.0;
          l = m = n = 0.0;
          return;
     }

     P_pa = 0.5 * rho_kgpm3 * pow(speed_air_world_mag_mps, 2);

     double surface_rad[4]; // 临时变量，不新增成员
     for (int i = 0; i < 4; i++)
     {
          surface_rad[i] = surface_d[i] * 180 / PI;
     }

     double omega_dimless[3]; // 仅函数内临时变量，不新增类成员
     for (int i = 0; i < 3; i++)
     {
          // 公式：无量纲角速度 = (有量纲角速度 × 参考长度) / (2 × 空速模值)
          omega_dimless[i] = (gyro_body_radps[i] * L_ref) / (2 * speed_air_world_mag_mps);
     }

     // 升力系数CL：攻角+俯仰角速度+舵面贡献（修正枚举索引为AIR_前缀）
     CL = C[AIR_AOA][AIR_L] * AOA +            // 攻角贡献
          C[AIR_Q][AIR_L] * omega_dimless[1] + // 俯仰角速度q（无量纲）
          C[AIR_FR][AIR_L] * surface_rad[0] +  // 前右舵面
          C[AIR_FL][AIR_L] * surface_rad[1] +  // 前左舵面
          C[AIR_BL][AIR_L] * surface_rad[2] +  // 后左舵面
          C[AIR_BR][AIR_L] * surface_rad[3];   // 后右舵面

     // 阻力系数CD：零升阻力+攻角二次项+舵面阻力
     CD = C[0][AIR_D] +                               // 零升阻力CD0（C[0][AIR_D]为零值系数）
          C[AIR_AOA][AIR_D] * pow(AOA, 2) +           // 诱导阻力（攻角平方）
          C[AIR_FR][AIR_D] * pow(surface_rad[0], 2) + // 舵面阻力（二次项）
          C[AIR_FL][AIR_D] * pow(surface_rad[1], 2) +
          C[AIR_BL][AIR_D] * pow(surface_rad[2], 2) +
          C[AIR_BR][AIR_D] * pow(surface_rad[3], 2);

     // 侧滑力系数CY：侧滑角+滚转/偏航角速度+舵面贡献
     CY = C[AIR_SOA][AIR_Y] * SOA +            // 侧滑角核心贡献
          C[AIR_P][AIR_Y] * omega_dimless[0] + // 滚转角速度p（无量纲）
          C[AIR_R][AIR_Y] * omega_dimless[2] + // 偏航角速度r（无量纲）
          C[AIR_FR][AIR_Y] * surface_rad[0] +
          C[AIR_FL][AIR_Y] * surface_rad[1] +
          C[AIR_BL][AIR_Y] * surface_rad[2] +
          C[AIR_BR][AIR_Y] * surface_rad[3];

     // 滚转力矩系数Cl：侧滑角+滚转/偏航角速度+舵面
     Cl = C[AIR_SOA][AIR_CL] * SOA +            // 侧滑角
          C[AIR_P][AIR_CL] * omega_dimless[0] + // 滚转角速度p（无量纲）
          C[AIR_R][AIR_CL] * omega_dimless[2] + // 偏航角速度r（无量纲）
          C[AIR_FR][AIR_CL] * surface_rad[0] +
          C[AIR_FL][AIR_CL] * surface_rad[1] +
          C[AIR_BL][AIR_CL] * surface_rad[2] +
          C[AIR_BR][AIR_CL] * surface_rad[3];

     // 俯仰力矩系数Cm：攻角+俯仰角速度+舵面
     Cm = C[AIR_AOA][AIR_CM] * AOA +            // 攻角
          C[AIR_Q][AIR_CM] * omega_dimless[1] + // 俯仰角速度q（无量纲）
          C[AIR_FR][AIR_CM] * surface_rad[0] +
          C[AIR_FL][AIR_CM] * surface_rad[1] +
          C[AIR_BL][AIR_CM] * surface_rad[2] +
          C[AIR_BR][AIR_CM] * surface_rad[3];

     // 偏航力矩系数Cn：侧滑角+滚转/偏航角速度+舵面
     Cn = C[AIR_SOA][AIR_CN] * SOA +            // 侧滑角
          C[AIR_P][AIR_CN] * omega_dimless[0] + // 滚转角速度p（无量纲）
          C[AIR_R][AIR_CN] * omega_dimless[2] + // 偏航角速度r（无量纲）
          C[AIR_FR][AIR_CN] * surface_rad[0] +
          C[AIR_FL][AIR_CN] * surface_rad[1] +
          C[AIR_BL][AIR_CN] * surface_rad[2] +
          C[AIR_BR][AIR_CN] * surface_rad[3];

     L = P_pa * S_ref * CL; // 升力
     D = P_pa * S_ref * CD; // 阻力
     Y = P_pa * S_ref * CY; // 侧滑力
     // 叠加风扇推力
     D -= this->fan_power_n;

     l = P_pa * S_ref * L_ref * Cl; // 滚转力矩
     m = P_pa * S_ref * L_ref * Cm; // 俯仰力矩
     n = P_pa * S_ref * L_ref * Cn; // 偏航力矩
}
