////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
//  Assignment:     Final Project
//  Instructor:     David Burchill
//  File name:      sfml.cpp
// 
//  Student name:   Ezequiel Flores Schmelensky
//  Student email:  ezequielfloressch@gmail.com
// 
//     I certify that this work is my work only, any work copied from Stack Overflow, textbooks, 
//     or elsewhere is properly cited. 
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////

//  BUG
//  list any and all bugs in your code 

#ifndef SFMLCLASS_ENTITYMANAGER_H
#define SFMLCLASS_ENTITYMANAGER_H

#include <vector>
#include <map>
#include <memory>
#include <string>

//forward declare
class Entity;

using NttPtr = std::shared_ptr<Entity>;
using EntityVec = std::vector<NttPtr>;
using EntityMap = std::map<std::string, EntityVec>;

class EntityManager {
private:
    EntityVec           m_entities;
    EntityMap           m_entityMap;
    size_t              m_totalEntites{0};

    EntityVec           m_entitiesToAdd;

    void removeDeadEntities(EntityVec &v);
public:
    EntityManager();

    NttPtr        addEntity(const std::string& tag);
    EntityVec&  getEntities();
    EntityVec&  getEntities(const std::string& tag);

    void update();

};


#endif //SFMLCLASS_ENTITYMANAGER_H
