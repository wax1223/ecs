#define ECSIMPL
#include "ecs.hpp"

struct Position
{
    Position(int _x, int _y)
    {
        x = _x;
        y = _y;
    }
    int x;
    int y;
};

struct Velocity
{
    Velocity(float _x, float _y)
    {
        x = _x;
        y = _y;
    }
    float x;
    float y;
};

struct PV
{
    Position p;
    Velocity v;
};

struct Base
{
    int a;
};
struct Deride :  Base
{
    float b;
};
struct Deridee: Deride
{
    Deridee(int _a, float _b, double _c)
    {
        a = _a;
        b = _b;
        c = _c;
    }
    double c;
};

class PosSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row& rows = *(GetRow("Position"));
        Position* p = (Position*)rows[0].comps[0];
        assert(p[0].x == 100 && p[0].y == 100);
        assert(p[1].x == 200 && p[1].y == 300);
    }
};


class VelSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row& rows = *GetRow("Velocity");
        Velocity* p = (Velocity*)rows[0].comps[0];
        assert((p[0].x - 1.23f) <= 0.000001 && (p[0].y - 2.123f) <= 0.000001);
        assert((p[1].x - 123.123f) <= 0.000001 && (p[1].y - 13.123f) <= 0.000001);
    }
};

class VelPosSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row& rows = *GetRow("Velocity ,  Position");
        Position* p = (Position*)rows[0].comps[0];
        Velocity* v = (Velocity*)rows[0].comps[1];
        assert(p[0].x == 5594 && p[0].y == 1314);
        assert((v[0].x - 1.21341f) <= 0.001 && (v[0].y - 213.123f) <= 0.001);
    }
};

class DSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row& rows = *GetRow("Deridee");
        Deridee* d = (Deridee*) rows[0].comps[0];
        assert(d[0].a == 123);
        assert(d[0].b == 0.123f);
        assert(d[0].c == 0.123321);
    }
};

class DPVSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row& rows = *GetRow("Deridee, Position, Velocity");
        cout <<"\n";
        for(int i = 0; i < rows.count; i++)
        {
            vector<Entity>& e = *rows[i].entitys;
            Deridee* d = (Deridee*)rows[0].comps[0];
            assert(d[0].a == 123);
            assert(d[0].b == 0.123f);
            assert(d[0].c == 0.123321);
            Position* ddp = (Position*)rows[0].comps[1];
            Velocity* ddv = (Velocity*)rows[0].comps[2];
            cout << "EntityCount: " << rows[i].entitys->size() << endl;
            for(int j = 0; j < rows[i].entitys->size(); j++)
            {
                cout << "#" << j << " eid: " << e[i].id << endl;
                cout << "ddpx: " << ddp[j].x++ << " ddpy: " << ddp[j].y ++<< endl;
                cout << "ddvx: " << ddv[j].x << " ddvy: " << ddv[j].y<< endl;
                ddv[j].x += 0.1f;
                ddv[j].y += 0.2f;
            }
        }
    }
};

int main(int argc, char const *argv[])
{
    RegisterComponent("Position", sizeof(Position));
    RegisterComponent("Velocity", sizeof(Velocity));
    RegisterComponent("PV", sizeof(PV));
    RegisterComponent("Deridee", sizeof(Deridee));

    EntityHandle mye =  NewEntity("Position");
    EntityHandle mye2 = NewEntity("Position");
    EntityHandle mye3 = NewEntity("Velocity");
    EntityHandle mye4 = NewEntity("Velocity");
    EntityHandle mye5 = NewEntity("Position, Velocity");
    EntityHandle mye6 = NewEntity("PV");

    EntityHandle myed = NewEntity("Deridee");
    EntityHandle myede = NewEntity("Deridee, Position, Velocity");


    Position* p = (Position*)GetCompPtr(mye,"Position");
    Position* p2 = (Position*)GetCompPtr(mye2,"Position");
    
    Velocity* p3 = (Velocity*)GetCompPtr(mye3,"Velocity");
    Velocity* p4 = (Velocity*)GetCompPtr(mye4,"Velocity");

    Position* p5p = (Position*)GetCompPtr(mye5,"Position");
    Velocity* p5v = (Velocity*)GetCompPtr(mye5,"Velocity");
    PV*       p6pv = (PV*)GetCompPtr(mye6, "PV");


    Deridee* d = (Deridee*) GetCompPtr(myed, "Deridee");
    Deridee* ddd = (Deridee*) GetCompPtr(myede, "Deridee");
    Position* ddp = (Position*) GetCompPtr(myede, "Position");
    Velocity* ddv = (Velocity*) GetCompPtr(myede, "Velocity");

    d->a = 123;
    d->b = 0.123f;
    d->c = 0.123321;

    ddd->a = 123;
    ddd->b = 0.123f;
    ddd->c = 0.123321;
    ddp->x = 123321;
    ddp->y = 1231;
    ddv->x = 1.1;
    ddv->y = 2.2;

    EntityHandle h[32];
    for(int i = 0; i < 20; i ++)
    {
        p->x = 100;
        p->y = 100;
        p2->x = 200;
        p2->y = p->x * 3;
        p3->x = 1.23f;
        p3->y = 2.123f;
        p4->x = 123.123f;
        p4->y = 13.123f;
        p5p->x =  5594;
        p5p->y =  1314;
        p5v->x = 1.21341f;
        p5v->y = 213.1233141f;

        p6pv->p.x = 123321;
        p6pv->p.y = 121212;
        p6pv->v.x = 0.123123f;
        p6pv->v.y = 0.321312f;

        p = (Position*)GetCompPtr(mye,"Position");
        p2 = (Position*)GetCompPtr(mye2,"Position");
    
        p3 = (Velocity*)GetCompPtr(mye3,"Velocity");
        p4 = (Velocity*)GetCompPtr(mye4,"Velocity");

        p5p = (Position*)GetCompPtr(mye5,"Position");
        p5v = (Velocity*)GetCompPtr(mye5,"Velocity");
        p6pv = (PV*)GetCompPtr(mye6, "PV");
        

        assert(p->x == 100);
        assert(p->y == 100);
        assert(p2->x == 200);
        assert(p2->y == p->x * 3);
        assert(p3->x == 1.23f);
        assert(p3->y == 2.123f);
        assert(p4->x == 123.123f);
        assert(p4->y == 13.123f);
        assert(p5p->x ==  5594);
        assert(p5p->y ==  1314);
        assert(p5v->x == 1.21341f);
        assert(p5v->y == 213.1233141f);
        assert(p6pv->p.x == 123321);
        assert(p6pv->p.y == 121212);
        assert(p6pv->v.x == 0.123123f);
        assert(p6pv->v.y == 0.321312f);
 
        PosSystem pos;
        pos.PrintData();
        VelSystem vol;
        vol.PrintData();
        VelPosSystem vp;
        vp.PrintData();
        DSystem dd;
        dd.PrintData();
        DPVSystem dpv;
        dpv.PrintData();

        h[i] = NewEntity("Deridee, Position, Velocity");

/*
        ddd = (Deridee*) GetCompPtr(h[i], "Deridee");
        ddp = (Position*) GetCompPtr(h[i], "Position");
        ddv = (Velocity*) GetCompPtr(h[i], "Velocity");
        ddd->a = 1;
        ddd->b = 0.1;
        ddd->c = 0.2;
        ddp->x = 1;
        ddp->y = 2;
        ddv->x = 0.01;
        ddv->y = 0.02;
        */
        Deridee td(1, 0.1f, 0.2);
        Position tp(1, 2);
        Velocity tv(0.01, 0.02);
        SetCompValue(h[i], "Deridee", (void*)&td);
        SetCompValue(h[i], "Position", (void*)&tp);
        SetCompValue(h[i], "Velocity", (void*)&tv);
    }

    return 0;
}