#ifndef GENERAL_HPP
#define GENERAL_HPP
/*重构这些参数，考虑这样的命名风格：
1.有物理意义的必须添加单位尾缀，比如质量 mass_kg 惯量I_kgm2 
2.有参考系歧义的必须标注参考系，比如速度 speed_world_mps[3]
3.如果是系数，比如 角度×系数=力矩，或者角度×系数=力 角度那么使用数组命名 C[aoa][L]不需要添加单位，即roll_d *C[aoa][L]=L(其中FL为aoa角度贡献的升力，)
4.方向定义：
机体系      x轴roll, 指向机头 顺时针为正（向右倾斜）
            y轴pitch,指向右翼，（抬头为正）
            z轴yaw,  指向下方，顺时针为正（机头右转）
其中在无风状态西，速度系=风轴系
因此升力L为速度系向上，D阻力速度系向后，侧滑力Y速度系向右
5.总共有4个舵面，前方右侧舵面角度提供的力矩和阻力：C[FR][ROLL],C[FR][D]
6.零值系数C0[F] C0[Y]...
7.角速度系数C[][]
8.
滚转力矩 (L),滚转角速度 p
俯仰力矩 (M),俯仰角速度 q
偏航力矩 (N),偏航角速度 r
尽量使用数组而非结构体，
*/


//S_ref = 0.0 // 参考面积 (Reference Area) - 例如：弹翼总面积
//L_ref = 0.0 // 参考长度 (Reference Length) - 例如：弹体长度

#define ROLL 0
#define PITCH 1
#define YAW 2

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2
//原因，来源
enum AIR_FROM{
    AIR_ROLL,//角度，或者别的，
    AIR_YAW,
    AIR_PITCH,
    AIR_P,// 滚转角速度 p（对应ROLL）
    AIR_Q,// 俯仰角速度 q（对应PITCH）
    AIR_R,// 偏航角速度 r（对应YAW）
    AIR_AOA,//攻角，影响力
    AIR_SOA,//侧滑角 
    AIR_FR, // 根据象限定义，前右第一象限。翼面力一般只和舵机角度相关
    AIR_FL,
    AIR_BL,
    AIR_BR,
    AIR_FROM_LAST
};
//结果，目标
enum AIR_POWER{
    AIR_L,//升力
    AIR_D,//阻力
    AIR_Y,//侧滑力
    AIR_CL,// 滚转力矩
    AIR_CM,// 俯仰力矩
    AIR_CN,// 偏航力矩
    AIR_POWER_LAST
};
#endif

