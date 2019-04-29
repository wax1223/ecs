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
        handle->column = entityCount = entitys.size() - 1;

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


/*
struct Row
{
    uint32_t typehash;
    uint32_t compCount;
    void** compsList;
};

//systemId to row
vector<Row> systemRow;
map<uint32_t, uint32_t> hashidTorowIndex;



Row& GetRow(uint32_t systemID)
{
    return systemRow[systemID];
}

uint32_t GetRow(string id)
{
    uint32_t hashid = hash32(id);
    Row r;
    auto row_i = hashidTorowIndex.find(r.typehash);
    if(row_i != hashidTorowIndex.end())
    {
        //find such row
        return systemRow[row_i];
    }
    else
    {

    }
}

uint32_t Match()
{
    //search row

    //if not such row, create one and match
    
    Row r;
    auto row_i = hashidTorowIndex.find(r.typehash);
    if(row_i != hashidTorowIndex.end())
    {
        //find such row
        return row_i;
    }
    else
    {
        //not found, create one

        r.compCount = compSeq.size();
        r.compsList = (void**) malloc(sizeof(void**) * r.compCount);

        //only contains!
        uint8_t row_index = HashidToEntityTableRow(r.typehash);
        auto ret = HashidToEntityTableRow.find(r.typehash);
        if(ret != HashidToEntityTableRow.end())
        {
            for(int i = 0; i < compSeq.size(); ++i)
            {
                for(int j = 0; j < EntityTable[row_index].columnsCount; ++j)
                {
                    if(compSeq == EntityTable[row_index].columns[j].compid);
                    {
                        r.compsList[i] = EntityTable[row_index].columns[j];
                        break;
                    }
                }
            }
        }
        //else not such type in

        //Todo(Wax) implement contains.

        systemRow.push_back(r);
        return systemRow.size() - 1;
    }
    return 0;
}

class System
{
public:
    uint32_t systemId;
    //component sequence
    vector<uint32_t> compSeq;
};

class PosSystem
{
public:
    void PrintData()
    {
        //OnlyOnce and use forever!
        Row& row = GetRow("Position, velocity");
        for(int i = 0; i < row.count; i++)
        {
            //Prepare for the dataArray you want to access;
            Position* p = row[i].column[0];
            for(int j = 0; j < row[i].columnCount; j++)
            {
                cout << "p->x: " << p[i]->x << " p->y: " << p[i]->y << endl;
            }
        }
    }
};

*/


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
    cout << "p5v->x: " << p5v->x << " p5v->y: " << p5v->y << endl;  

    cout << "p6pv->p.x: " << p6pv->p.x
     << " p6pv->p.y: " << p6pv->p.y
     << " p6pv->v.x: " << p6pv->v.x
     << " p6pv->v.y: " << p6pv->v.y;
    
    return 0;
}