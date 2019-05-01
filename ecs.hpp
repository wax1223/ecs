#pragma once

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <random>
#include <algorithm>

using namespace std;

inline uint32_t hash32(string str);
uint32_t GetComponentID(string name);


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
        assert(d);
        assert(cur < arrSize * elemSize);
        
        memcpy((void*)(&data[cur]), d, elemSize);
        elemCount++;
        return (void*)(&data[cur]);
    }
    void SetValue(int elemindex, void* val)
    {
        int cur = GetCursor(elemindex);
        assert(cur < arrSize * elemSize);
        memcpy((void*)(&data[cur]), val, elemSize);
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


struct Component_t
{
    uint32_t compSize:16;
    uint32_t entityTableRow:16;
};

typedef uint32_t EntityID;

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


struct EntityList
{
    uint32_t typehash;
    uint8_t columnsCount = 0;
    uint32_t* compids;
    SizeArray** datas;
    std::vector<Entity> entitys;

    EntityList(uint32_t _typehash, std::vector<string>& cL);


    const EntityList& operator= (const EntityList& other)
    {
        assert(0);
    }

    EntityList(EntityList& other) : typehash(other.typehash),
                    columnsCount(other.columnsCount)
    {
        this->compids = other.compids;
        this->datas = other.datas;
        this->entitys = other.entitys;
        other.compids = NULL;
        other.datas = NULL;
    }

    EntityList(EntityList&& other) : typehash(other.typehash),
                    columnsCount(other.columnsCount)
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

    void AddEntity(EntityHandle* handle, void* valuePtr = NULL)
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
        handle->column = entitys.size() - 1;

        for(int i = 0; i < columnsCount; i++)
        {
            if(!valuePtr) 
            {
                datas[i]->PushElem();
            }
            else 
            {
                datas[i]->PushElem(valuePtr);
            }
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
    void SetEntityValue(int index, uint32_t compid, void* value)
    {
        for(int i = 0; i < columnsCount; i++)
        {
            if(compid == compids[i])
            {
                datas[i]->SetValue(index, value);
                break;
            }
        }
    }
};


struct inner
{
    uint32_t typehash[16];
    uint8_t columnCount = 0;
    void* comps[16];
    vector<Entity>* entitys;
};

struct Row
{
    int count = 0;
    inner* eachrow = NULL;

    inner& operator[](int i)
    {
        return eachrow[i];
    }
    ~Row()
    {
        if(eachrow) free(eachrow);
        eachrow = NULL;
    }
    void operator= (Row& other)
    {
        assert(0);
        count = other.count;
        eachrow = other.eachrow;
        other.eachrow = NULL;
    }
    Row() = default;
    Row(const Row&& other) = delete;
    Row(const Row& other) = delete;
};

static_assert(sizeof(EntityHandle) == 8, "EntityHandle should equal to 8");
static_assert(sizeof(Entity) == 32, "Entity should equal to 8");

void RegisterComponent(string name, uint16_t elemsize);
void* GetCompPtr(EntityHandle& entityid, string type);
void SetCompValue(EntityHandle& entityid, string type, void* valuePtr);
void AddComponent(uint64_t eid, uint32_t compid);
void RemoveComponent(uint64_t eid, uint32_t compid);
void DeleteEntity(uint64_t eid);
EntityHandle NewEntity(uint32_t row, void* valuePtr = NULL);
EntityHandle NewEntity(string types, void* valuePtr = NULL);
Row* GetRow(string id);
string Trim(const string& s);
void SeperateBy(string str, char seperator, vector<string>& substrs);



#ifdef ECSIMPL
random_device rd;
uniform_int_distribution<uint32_t> dist(0);
std::hash<string> hasher;

inline uint32_t hash32(string str)
{
    return uint32_t(hasher(str));
}

uint32_t GetComponentID(string name)
{
    return hash32(name);
}

map<uint32_t, Component_t> compsTable;
vector<EntityList> EntityTable;
//systemId to row
map<uint32_t, Row> hashidTorowIndex;


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

string joinString(vector<string>& str)
{
    string s;
    for(int i = 0; i < str.size(); i++)
    {
        s += str[i];
    }
    return s;
}

EntityList::EntityList(uint32_t _typehash, std::vector<string>& cL)
{
    typehash = _typehash;
    columnsCount = cL.size();
    compids = (uint32_t*)malloc(columnsCount * sizeof(uint32_t));
    datas = (SizeArray**)malloc(columnsCount * sizeof(SizeArray*));
    for(int i = 0; i < columnsCount; i++)
    {
        uint32_t hashid = GetComponentID(cL[i]);
        uint16_t elemSize = compsTable[hashid].compSize;
        compids[i] = hashid;
        datas[i] = new SizeArray(elemSize, 32);
    }
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
    }
    //not found;
    assert(0);
    return NULL;
}

void SetCompValue(EntityHandle& entityid, string type, void* valuePtr)
{
    Entity& e = EntityTable[entityid.row].entitys[entityid.column];
    if(entityid.id == e.id)
    {
        EntityTable[entityid.row].SetEntityValue(entityid.column, GetComponentID(type), valuePtr);
    }
    else
    {
        //relocate entity position
        assert(0);
    }
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


EntityHandle NewEntity(uint32_t row, void* valuePtr)
{
    EntityHandle h;
    h.row = row;
    h.id = dist(rd);
    EntityTable[row].AddEntity(&h, valuePtr);
    return h;
}

EntityHandle NewEntity(string types, void* valuePtr)
{
    vector<string> cL;
    SeperateBy(types, ',', cL);
    sort(cL.begin(), cL.end(), [](string& a, string& b)
    {
        int i = 0;
        while(a[i] == b[i]) i++;
        return a[i] < b[i];
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

    return NewEntity(row, valuePtr);
}


Row* GetRow(string id)
{
    vector<string> cL;
    SeperateBy(id, ',', cL);
    sort(cL.begin(), cL.end(), [](string& a, string& b)
    {
        int i = 0;
        while(a[i] == b[i]) i++;
        return a[i] < b[i];
    });
    string sstr = joinString(cL);
    uint32_t hashid = hash32(sstr);

    uint32_t row[32];
    uint32_t rowc = 0;

    auto ret = hashidTorowIndex.find(hashid);
    if(ret != hashidTorowIndex.end())
    {
        return &ret->second;
    }
    else
    {
        Row& r = hashidTorowIndex[hashid];

        //matching equal.
        auto cret = compsTable.find(hashid);
        if(cret != compsTable.end())
        {
            row[rowc++] = cret->second.entityTableRow;
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
            }
        }
        r.count = rowc;
        r.eachrow = (inner*)malloc(sizeof(inner) * r.count);
        for(int i = 0; i < r.count; i++)
        {
            r.eachrow[i].entitys = &(EntityTable[row[i]].entitys);
            r.eachrow[i].columnCount = EntityTable[row[i]].columnsCount;
            for(int j = 0; j < r.eachrow[i].columnCount; j++)
            {
                r.eachrow[i].comps[j] = EntityTable[row[i]].datas[j]->data;
                r.eachrow[i].typehash[j] = EntityTable[row[i]].compids[j];
            }
        }
        return &r;
    }
}
#endif