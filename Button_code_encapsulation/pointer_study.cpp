#include <stdio.h>

/*
 sp->age 是更常用的简写，和 (*sp).age 完全等价
*/
typedef struct
{
    int age;
    int score;
} Student;

void change(int *p)
{
    // *p 表示访问 p 指向的那块内存中的值
    // 因为 p 保存的是外部变量的地址，所以这里会直接改到原变量
    *p = 100;
}

int main()
{
    printf("========== 第一部分：普通指针 ==========\n");
    int a = 10;             // 定义普通整型变量 a，并赋初值 10
    printf("a值: %d\n", a); // 输出变量 a 当前的值

    // &a 表示取 a 的地址   %p 用于打印地址，由int*转换成 void* 更规范
    printf("地址a: %p\n", (void *)&a);
    printf("-----------------------------------\n");

    // 定义整型指针 p
    // int *p 的含义是：p 是一个“指向 int 的指针”
    int *p = &a;                               //&a 即将a的地址赋给 p，说明 p 指向 a的地址
    printf("通过指针访问（*p)的值: %d\n", *p); // *p 表示取出 p 指向的地址里面的值

    // 这里输出的是指针变量 p 自己的地址，不是 p 保存的地址
    printf("地址p: %p\n", (void *)&p);
    printf("-----------------------------------\n");

    // 通过指针修改 a 的值
    // p 里保存的是 a 的地址，所以 *p = 20 等价于 a = 20
    *p = 20;
    printf("修改后的值: %d\n", a);
    printf("-----------------------------------\n");

    // 把 a 的地址传给函数 change
    // 函数内部通过这个地址，把 a 改成 100
    change(&a);
    printf("change后的值: %d\n", a);
    printf("change后地址a: %p\n", (void *)&a);
    printf("-----------------------------------\n\n");

    printf("========== 第二部分：结构体与结构体指针 ==========\n");

    // 定义一个结构体变量 s1
    Student s1;

    // 给结构体成员赋初值
    s1.age   = 18;
    s1.score = 90;

    // 直接通过结构体变量访问成员，使用 . 运算符
    printf("初始 Student age: %d\n", s1.age);
    printf("初始 Student score: %d\n", s1.score);
    printf("结构体变量 s1 的地址: %p\n", (void *)&s1);
    printf("-----------------------------------\n");

    // 定义结构体指针 sp，并让它指向 s1
    Student *sp = &s1;

    // 通过结构体指针修改成员
    // sp->age 等价于 (*sp).age
    sp->age   = 20;
    sp->score = 95;

    // 通过结构体变量读取修改后的成员
    printf("通过 s1 读取 age: %d\n", s1.age);
    printf("通过 s1 读取 score: %d\n", s1.score);
    printf("-----------------------------------\n");

    // 通过 (*sp).成员 的写法访问
    printf("通过 (*sp).age 读取: %d\n", (*sp).age);
    printf("通过 (*sp).score 读取: %d\n", (*sp).score);
    printf("-----------------------------------\n");

    // 通过 sp->成员 的写法访问
    // 这是结构体指针最常见、最简洁的写法
    printf("通过 sp->age 读取: %d\n", sp->age);
    printf("通过 sp->score 读取: %d\n", sp->score);

    printf("结构体指针 sp 自己的地址: %p\n", (void *)&sp);
    printf("sp 保存的内容（也就是 s1 的地址）: %p\n", (void *)sp);

    printf("结构体变量 s1 的地址: %p\n", (void *)&s1);
    printf("-----------------------------------\n");

    return 0;
}
