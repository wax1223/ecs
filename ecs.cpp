#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <random>
#include <algorithm>

using namespace std;

random_device rd;

uniform_int_distribution<uint32_t> dist(0);

class SizeArray
{
public:
    int elemSize;
    int arrSize;
    int elemCount;
    uint8_t* data;
    SizeArray(SizeArray& other) :
        elemSize(other.elemSize), arrSize(other.arrSize),
        elemCount(other.elemCount)
    {
        this->data = other.data;
        other.data = NULL;
    }
    SizeArray(SizeArray&& other)
    {
        this->data = other.data;
        other.data = NULL;
    }
    SizeArray& operator=(SizeArray other)
    {
        assert(0);
    }
    SizeArray(int _elemSize, int _arrSize)
    {
        elemSize = _elemSize;
        arrSize = _arrSize;
        elemCount = 0;
        data = (uint8_t*)malloc(_elemSize * _arrSize);
    }
    ~SizeArray()
    {
        if(data)
        {
            free(data);
            data = NULL;
        }
    }
    int GetCursor(int elemindex)
    {
        return elemindex * elemSize;
    }
    void* PushElem(void * d)
    {
        int cur = GetCursor(elemCount);

        assert(cur < arrSize * elemSize);
        
        memcpy((void*)(&data[cur]), d, elemSize);
        elemCount++;
        return (void*)(&data[cur]);
    }

    void PushElem()
    {
        int cur = GetCursor(elemCount);
        assert(cur < arrSize * elemSize);
        elemCount++;
    }
    
    template <typename T>
    T* GetElem(int index)
    {
        int cur = GetCursor(index);
        assert(cur < arrSize * elemSize);
        return (T*)(&data[cur]);
    }
    void* operator[](int index)
    {
        int cur = GetCursor(index);
        assert(cur < arrSize * elemSize);
        return &data[cur];
    }
    void DelElem(int index)
    {
        int cur = GetCursor(index);
        assert(cur < arrSize * elemSize);
        assert(cur >= 0);

        int lastone = elemCount - 1;
        if(index != lastone) //lastone 
        {
            //CoverWith the last one
            int lastOneCur = GetCursor(lastone);
            memcpy((void*)&data[cur], (void*)&data[lastOneCur], elemSize);
        }
        elemCount--;
    }
};


string Trim(const string& s)
{
    int firstNotSpace = 0;
    for(; firstNotSpace < s.size(); firstNotSpace++)
    {
        if(s[firstNotSpace] != ' ' && s[firstNotSpace] != '\t' && s[firstNotSpace] != '\n')
        {
            break;
        }
    }
    int lastNotSpace = s.size() - 1;
    for(; lastNotSpace >= 0; lastNotSpace--)
    {
        if(s[lastNotSpace] != ' ' && s[lastNotSpace] != '\t' && s[lastNotSpace] != '\n')
        {
            break;
        }
    }
    return s.substr(firstNotSpace, lastNotSpace - firstNotSpace + 1);
}

void SeperateBy(string str, char seperator, vector<string>& substrs)
{
    string::size_type cur, last = 0;

    while(true)
    {
        cur = str.find(seperator, last);
        string s = str.substr(last, cur - last);

        substrs.push_back(s);

        if(cur == string::npos)
        {
            break;
        }
        last = cur + 1; //skip seperator;
    }

    for(string& s : substrs)
    {
        s = Trim(s);
    }
    //Todo(Wax): remove all the empty str;
}

typedef uint32_t EntityID;

std::hash<string> hasher;

inline uint32_t hash32(string str)
{
    return uint32_t(hasher(str));
}

struct Component_t
{
    uint32_t compSize:16;
    uint32_t entityTableRow:16;
};

map<uint32_t, Component_t> compsTable;


union EntityHandle
{
    uint64_t value;
    struct
    {
        EntityID id;
        uint32_t row : 12;
        uint32_t column : 20;
    };
};

struct Entity
{
    EntityHandle parent;
    EntityHandle leftMostChild;
    EntityHandle sibling;
    EntityID id;
};


static_assert(sizeof(EntityHandle) == 8, "EntityHandle should equal to 8");
static_assert(sizeof(Entity) == 32, "Entity should equal to 8");


uint32_t GetComponentID(string name)
{
    return hash32(name);
}

struct EntityList
{
    uint32_t typehash;
    uint8_t columnsCount = 0;
    uint32_t* compids;
    SizeArray** datas;
    uint32_t entityCount = 0;
    std::vector<Entity> entitys;
    EntityList(uint32_t _typehash, std::vector<string>& cL)
    {
        typehash = _typehash;
        columnsCount = cL.size();
        compids = (uint32_t*)malloc(columnsCount * sizeof(uint32_t*));
        datas = (SizeArray**)malloc(columnsCount * sizeof(SizeArray**));
        for(int i = 0; i < columnsCount; i++)
        {
            uint32_t hashid = GetComponentID(cL[i]);
            uint16_t elemSize = compsTable[hashid].compSize;
            compids[i] = hashid;
            datas[i] = new SizeArray(elemSize, 8);
        }
    }

    const EntityList& operator= (const EntityList& other)
    {
        assert(0);
    }

    EntityList(EntityList& other) : typehash(other.typehash),
                    columnsCount(other.columnsCount), entityCount(other.columnsCount)
    {
        this->compids = other.compids;
        this->datas = other.datas;
        this->entitys = other.entitys;
        other.compids = NULL;
        other.datas = NULL;
    }

    EntityList(EntityList&& other) : typehash(other.typehash),
                    columnsCount(other.columnsCount), entityCount(other.columnsCount)
    {
        this->compids = other.compids;
        this->datas = other.datas;
        this->entitys = other.entitys;
        other.compids = NULL;
        other.datas = NULL;
    }
    
    ~EntityList()
    {
        if(compids != NULL) free(compids);
        if(datas != NULL)
        {
            for(int i = 0; i < columnsCount; i++)
            {
                if(datas[i] != NULL)
                {
                    delete(datas[i]);
                    datas[i] = NULL;
                }
            }
            free(datas);
            datas = NULL;
        }
    }

    void AddEntity(EntityHandle* handle)
    {
        //Todo(Wax): no space to store!
        // *handle
        Entity e;
        EntityHandle h;
        h.value = 0;
        e.id = handle->id;
        e.leftMostChild = h;
        e.parent = h;
        e.sibling = h;
        entitys.push_back(e);
        entityCount = entitys.size();
        handle->column = entityCount - 1;

        for(int i = 0; i < columnsCount; i++)
        {
            datas[i]->PushElem();
        }
    }

    void* GetEntityComponet(int index, uint32_t compid)
    {
        for(int i = 0; i < columnsCount; i++)
        {
            if(compid == compids[i])
            {
                return (*(datas[i]))[index];
            }
        }
        assert(0);
        //assertion for not found?
    }
};

vector<EntityList> EntityTable;

string joinString(vector<string>& str)
{
    string s;
    for(int i = 0; i < str.size(); i++)
    {
        s += str[i];
    }
    return s;
}

void RegisterComponent(string name, uint16_t elemsize)
{
    //Todo(Wax): add not register component will trigger an assertion.

    // search table with componentList
    uint32_t hashid = hash32(name);

    //SearchTable;
    auto ret = compsTable.find(hashid);
    if(ret == compsTable.end())
    {
        //Not found
        compsTable[hashid] = {elemsize, 0};
        // if not match create a new EntityList
        std::vector<string> vname;
        vname.push_back(name);
        EntityTable.push_back(EntityList(hashid, vname));
        compsTable[hashid].entityTableRow = EntityTable.size() - 1;
    }
    //found?
    //TODO(wax): Give out a warning?
}

void* GetCompPtr(EntityHandle& entityid, string type)
{
    
    Entity e = EntityTable[entityid.row].entitys[entityid.column];
    if(entityid.id == e.id)
    {
        return EntityTable[entityid.row].GetEntityComponet(entityid.column, GetComponentID(type));
    }
    else
    {
        //relocate entity position
        assert(0);
        /*
        auto row = HashidToEntityTableRow[eid.typehash];
        for(int i = 0; i < EntityTable[eid.row].entityCount; i++)
        {
            if((*(EntityTable[eid.row].columns[0].data)).GetElem<EntityHandle>().randomValue == eid.randomValue)
            {
                eid.row = row;
                eid.column = i;
                //update entityid value;
                entityid = eid.id;

                return EntityTable[eid.row].GetEntityComponet(eid.column, compid);
            }
        }
        */
    }
    //not found;
    assert(0);
    return NULL;
}

void AddComponent(uint64_t eid, uint32_t compid)
{

}

void RemoveComponent(uint64_t eid, uint32_t compid)
{

}

void DeleteEntity(uint64_t eid)
{

}


struct Position
{
    int x;
    int y;
};


struct Velocity
{
    float x;
    float y;
};


struct Row
{
    int count;
    struct inner
    {
        uint32_t typehash[16];
        int entityCount;
        int columnCount;
        void* comps[16];
        vector<Entity> * entitys;
    };
    inner* eachrow;
    inner& operator[](int i)
    {
        return eachrow[i];
    }
    ~Row()
    {
        if(eachrow) free(eachrow);
        eachrow = NULL;
    }
    Row() {}
    void operator= (const Row& other)
    {
        assert(0);
    }

    // Row(const Row&& other) : count(other.count)
    // {
    //     // assert(0);
    //     eachrow = other.eachrow;
    //     // Row* r = const_cast<Row*>(other);
    //     // other.eachrow = NULL;
    // }
    Row(const Row&& other) = delete;

    Row(const Row& other) = delete;
    // {
    //     eachrow = other.eachrow;
    //     Row* r = const_cast<Row*>(&other);
    //     r->eachrow = NULL;
    // }
};

//systemId to row
map<uint32_t, Row> hashidTorowIndex;

void GetRow(Row& r, string id)
{
    vector<string> cL;
    SeperateBy(id, ',', cL);
    sort(cL.begin(), cL.end(), [](string& a, string& b)
    {
        return a[0] < b[0];
    });
    string sstr = joinString(cL);
    uint32_t hashid = hash32(sstr);

    uint32_t row[32];
    uint32_t rowc = 0;

    auto ret = hashidTorowIndex.find(hashid);
    if(ret != hashidTorowIndex.end())
    {
        //find such row
        r =  ret->second;
    }
    else
    {
        //matching equal.
        auto cret = compsTable.find(hashid);
        if(cret != compsTable.end())
        {
            row[rowc++] = cret->second.entityTableRow;
            r.count = rowc;
        }
        else
        {
            if(false)
            {
                //TODO(wax):  contains? negate?
            }
            else
            {
                            //Not match any entityrow.
                r.count = 0;
            }
        }
    }
    r.eachrow = (Row::inner*)malloc(sizeof(Row::inner*) * r.count);
    for(int i = 0; i < r.count; i++)
    {
        r.eachrow[i].entitys = &EntityTable[row[i]].entitys;
        r.eachrow[i].entityCount = EntityTable[row[i]].entityCount;
        r.eachrow[i].columnCount = EntityTable[row[i]].columnsCount;
        for(int j = 0; j <  r.eachrow[i].columnCount; j++)
        {
            r.eachrow[i].comps[j] = EntityTable[row[i]].datas[j]->data;
            r.eachrow[i].typehash[j] = EntityTable[row[i]].compids[j];
        }
    }
}

class PosSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row rows;
        GetRow(rows, "Position");
    
        cout << "\n\n";
        for(int i = 0; i < rows.count; i++)
        {
            cout << "\n";
            vector<Entity>& e = *rows[i].entitys;
            Position* p = (Position*)rows[i].comps[0];
            for(int j = 0; j < rows[i].entityCount; j++)
            {
                cout << "Entity id: " << e[j].id << " with value: " << endl;
                cout << "p->x: " << p[j].x << " p->y: " << p[j].y << endl;
            }
        }
    }
};

class VelSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row rows;
        GetRow(rows, "Velocity");
    
        cout << "\n\n";
        for(int i = 0; i < rows.count; i++)
        {
            cout << "\n";
            vector<Entity>& e = *rows[i].entitys;
            Velocity* p = (Velocity*)rows[i].comps[0];
            for(int j = 0; j < rows[i].entityCount; j++)
            {
                cout << "Entity id: " << e[j].id << " with value: " << endl;
                cout << "v->x: " << p[j].x << " v->y: " << p[j].y << endl;
            }
        }
    }
};

class VelPosSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row rows;
        GetRow(rows, "Velocity ,  Position");
    
        cout << "\n\n";
        for(int i = 0; i < rows.count; i++)
        {
            cout << "\n";
            vector<Entity>& e = *rows[i].entitys;
            Position* p = (Position*)rows[i].comps[0];
            Velocity* v = (Velocity*)rows[i].comps[1];
            for(int j = 0; j < rows[i].entityCount; j++)
            {
                cout << "Entity id: " << e[j].id << " with value: " << endl;
                cout << "p->x: " << p[j].x << " p->y: " << p[j].y << endl;
                cout << "v->x: " << v[j].x << " v->y: " << v[j].y << endl;
            }
        }
    }
};


EntityHandle NewEntity(uint32_t row)
{
    EntityHandle h;
    h.row = row;
    h.id = dist(rd);
    EntityTable[row].AddEntity(&h);
    return h;
}

EntityHandle NewEntity(string types, int count = 1)
{
    vector<string> cL;
    SeperateBy(types, ',', cL);
    sort(cL.begin(), cL.end(), [](string& a, string& b)
    {
        return a[0] < b[0];
    });
    string sstr = joinString(cL);
    uint32_t hashid = hash32(sstr);

    uint32_t row;
    //SearchTable;
    auto ret = compsTable.find(hashid);
    if(ret != compsTable.end())
    {
        //Found
        row = ret->second.entityTableRow;
    }
    else
    {
        // Not found
        // if not match create a new table with EntityList
        EntityTable.push_back(EntityList(hashid, cL));
        row =  EntityTable.size() - 1;
        compsTable[hashid] = {0, row};
    }

    return NewEntity(row);
}

struct PV
{
    Position p;
    Velocity v;
};

int main(int argc, char const *argv[])
{
    RegisterComponent("Position", sizeof(Position));
    RegisterComponent("Velocity", sizeof(Velocity));
    RegisterComponent("PV", sizeof(PV));

    EntityHandle mye =  NewEntity("Position");
    EntityHandle mye2 = NewEntity("Position");
    EntityHandle mye3 = NewEntity("Velocity");
    EntityHandle mye4 = NewEntity("Velocity");
    EntityHandle mye5 = NewEntity("Position, Velocity");
    EntityHandle mye6 = NewEntity("PV");

    Position* p = (Position*)GetCompPtr(mye,"Position");
    Position* p2 = (Position*)GetCompPtr(mye2,"Position");
    
    Velocity* p3 = (Velocity*)GetCompPtr(mye3,"Velocity");
    Velocity* p4 = (Velocity*)GetCompPtr(mye4,"Velocity");

    Position* p5p = (Position*)GetCompPtr(mye5,"Position");
    Velocity* p5v = (Velocity*)GetCompPtr(mye5,"Velocity");
    PV*       p6pv = (PV*)GetCompPtr(mye6, "PV");

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
    p6pv->v.x = 0.123123;
    p6pv->v.y = 0.321312;

    p = (Position*)   GetCompPtr(mye2,   "Position");
    p2 = (Position*)  GetCompPtr(mye,   "Position");
    p3 = (Velocity*)  GetCompPtr(mye4,  "Velocity");
    p4 = (Velocity*)  GetCompPtr(mye3,  "Velocity");
    p5p = (Position*) GetCompPtr(mye5, "Position");
    p5v = (Velocity*) GetCompPtr(mye5, "Velocity");
    p6pv = (PV*)GetCompPtr(mye6, "PV");

    cout << "p->x: " << p->x << " p->y: " << p->y << endl;
    cout << "p2->x: " << p2->x << " p2->y: " << p2->y << endl;
    cout << "p3->x: " << p3->x << " p3->y: " << p3->y << endl;
    cout << "p4->x: " << p4->x << " p4->y: " << p4->y << endl;
    cout << "p5p->x: " << p5p->x << " p5p->y: " << p5p->y << endl;
    cout << "p5v->x: " << p5v->x << " p5v->y: " << p5v->y << endl;

    cout << "p6pv->p.x: " << p6pv->p.x
     << " p6pv->p.y: " << p6pv->p.y
     << " p6pv->v.x: " << p6pv->v.x
     << " p6pv->v.y: " << p6pv->v.y;
    

    PosSystem pos;
    pos.PrintData();
    VelSystem vol;
    vol.PrintData();
    VelPosSystem vp;
    vp.PrintData();

    return 0;
}