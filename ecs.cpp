#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <random>

using namespace std;

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


void TestSeperateBy()
{
    string a = "Position , Conves, adf, a ssd   , aadfasdf";
    vector<string> v;
    SeperateBy(a, ',', v);
    for(auto s : v)
    {
        cout << '|' << s << "|\n";
    }
    cout << endl;
}


struct ComponentArray
{
    SizeArray* data;
    uint32_t compid;
};


struct Entity
{
    /*
    hash
    name
    parent
    childlist
    */
};


std::hash<string> hasher;

inline uint32_t hash32(string str)
{
    return uint32_t(hasher(str));
}

struct TypeDesc
{
    uint32_t id;
    uint16_t size;
};


map<uint32_t, TypeDesc> compTable;
map<uint32_t, uint8_t> HashidToEntityTableRow;


union EntityHandle
{
    uint64_t id;
    struct
    {
        uint8_t randomValue;
        uint8_t row;
        uint16_t column;
        uint32_t typehash;
    };
};
static_assert(sizeof(EntityHandle) == 8, "EntityHandle should equal to 8");


uint32_t GetComponentID(string name)
{
    return hash32(name);
}

struct EntityList
{
    uint32_t typehash;
    ComponentArray* columns;
    uint8_t columnsCount = 0;
    uint32_t entityCount = 0;
    EntityList(uint32_t _typehash, vector<string>& cL)
    {
        columnsCount = (uint8_t)cL.size() + 1;
        columns = (ComponentArray*)malloc(columnsCount * sizeof(ComponentArray));
        columns[0].data = new SizeArray(sizeof(EntityHandle), 8);
        columns[0].compid = _typehash;
        for(int i = 1; i < columnsCount; i++)
        {
            TypeDesc& td = compTable[GetComponentID(cL[i - 1])];
            columns[i].data = new SizeArray(td.size, 8);
            columns[i].compid = td.id;
        }
    }
    const EntityList& operator= (const EntityList& other)
    {
        assert(0);
    }
    EntityList(EntityList& other) : typehash(other.typehash),
                    columnsCount(other.columnsCount), entityCount(other.columnsCount)
    {
        this->columns = other.columns;
        other.columns = NULL;
    }
    EntityList(EntityList&& other) : typehash(other.typehash),
                    columnsCount(other.columnsCount), entityCount(other.columnsCount)
    {

        this->columns = other.columns;
        other.columns = NULL;
    }
    ~EntityList()
    {
        if(columns != NULL)
        {
            for(int i = 0; i < columnsCount; i++)
            {
                if(columns[i].data != NULL)
                {
                    delete(columns[i].data);
                    columns[i].data = NULL;
                }
            }
            free(columns);
            columns = NULL;
        }
    }

    void NewEntity(EntityHandle* handle)
    {
        columns[0].compid = handle->typehash;
        //Todo(Wax): no space to store!
        handle->column = columns[0].data->elemCount;
        columns[0].data->PushElem(handle);
        for(int i = 1; i < columnsCount; i++)
        {
            columns[i].data->PushElem();
        }
        entityCount++;
    }

    void* GetEntityComponet(int index, uint32_t compid)
    {
        for(int i = 1; i < columnsCount; i++)
        {
            if(compid == columns[i].compid)
            {
                return (*(columns[i].data))[index];
            }
        }
        assert(0);
        //assertion for not found?
    }
};

vector<EntityList> EntityTable;


void RegisterComponent(string name, uint16_t size)
{
    uint32_t h = hash32(name);
    compTable[h] = TypeDesc{h, size};
}


uint64_t RegisterEntity(string compList)
{
    //Todo(Wax): add not register component will trigger an assertion.
    uint32_t hashid = hash32(compList);

    // search table with componentList
    vector<string> cL;
    SeperateBy(compList, ',', cL);
    EntityHandle eid;

    //SearchTable;
    auto ret = HashidToEntityTableRow.find(hashid);
    if(ret != HashidToEntityTableRow.end())
    {
        //Found 
        // if match return a new instance in that table
        uint8_t row = ret->second;
        EntityList& enL = EntityTable[row];
        enL.NewEntity(&eid);
        eid.row = row;
    }
    else
    {
        // if not match create a new table with componentList
        //Not found
        EntityTable.push_back(EntityList(hashid, cL));
        int lastElemIndex =  EntityTable.size() - 1;

        uint8_t row = lastElemIndex;
        HashidToEntityTableRow[hashid] = row;

        eid.row = row;
        eid.typehash = hashid;
        eid.randomValue = dist(rd);
        EntityTable[row].NewEntity(&eid);
    }

    return eid.id;
}

void* GetCompPtr(uint64_t& entityid, uint32_t compid)
{
    EntityHandle eid;
    eid.id = entityid;
    assert(eid.id >= 0);
    assert(eid.row >= 0);
    assert(eid.column >= 0);
    ComponentArray& compArr = EntityTable[eid.row].columns[0];
    SizeArray& sa = *(compArr.data);
    EntityHandle* el = sa.GetElem<EntityHandle>(eid.column);
     
    //find row
    if(eid.row < EntityTable.size()  
        && eid.column < EntityTable[eid.row].entityCount
        && el->id == eid.id)
    {
        return EntityTable[eid.row].GetEntityComponet(eid.column, compid);
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



// struct Row
// {
//     uint32_t Count;
//     uint32_t* rows;
// }


// class PosSystem
// {
// public:
//     uint16_t systemId;
//     Row row;
//     void PrintData()
//     {
//         for(int i = 0; i < row.count; i++)
//         {
//             //Prepare for the dataArray you want to access;
//             row.rows[i]
//             Position* p = getRowPtr(Position);

//             for(int j = 0; j < ; j++)
//             {
//                 cout << "p->x: " << p[i]->x << " p->y: " << p[i]->y << endl;
//             }
//         }
//     }
// };


int main(int argc, char const *argv[])
{
    RegisterComponent("Position", sizeof(Position));
    RegisterComponent("Velocity", sizeof(Velocity));

    uint64_t mye = RegisterEntity("Position");
    uint64_t mye2 = RegisterEntity("Position");
    uint64_t mye3 = RegisterEntity("Velocity");
    uint64_t mye4 = RegisterEntity("Velocity");
    uint64_t mye5 = RegisterEntity("Position, Velocity");

    Position* p = (Position*)GetCompPtr(mye, GetComponentID("Position"));
    Position* p2 = (Position*)GetCompPtr(mye2, GetComponentID("Position"));
    
    Velocity* p3 = (Velocity*)GetCompPtr(mye3, GetComponentID("Velocity"));
    Velocity* p4 = (Velocity*)GetCompPtr(mye4, GetComponentID("Velocity"));

    Position* p5p = (Position*)GetCompPtr(mye5, GetComponentID("Position"));
    Velocity* p5v = (Velocity*)GetCompPtr(mye5, GetComponentID("Velocity"));

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

    p = (Position*)GetCompPtr(mye2, GetComponentID("Position"));
    p2 = (Position*)GetCompPtr(mye, GetComponentID("Position"));

    p3 = (Velocity*)GetCompPtr(mye4, GetComponentID("Velocity"));
    p4 = (Velocity*)GetCompPtr(mye3, GetComponentID("Velocity"));
    p5p = (Position*)GetCompPtr(mye5, GetComponentID("Position"));
    p5v = (Velocity*)GetCompPtr(mye5, GetComponentID("Velocity"));

    cout << "p->x: " << p->x << " p->y: " << p->y << endl;
    cout << "p2->x: " << p2->x << " p2->y: " << p2->y << endl;
    cout << "p3->x: " << p3->x << " p3->y: " << p3->y << endl;
    cout << "p4->x: " << p4->x << " p4->y: " << p4->y << endl;
    cout << "p5p->x: " << p5p->x << " p5p->y: " << p5p->y << endl;
    cout << "p5v->x: " << p5v->x << " p5v->y: " << p5v->y << endl;

    return 0;
}


