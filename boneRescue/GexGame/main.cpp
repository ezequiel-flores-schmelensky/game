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

//  sfml.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include "GameEngine.h"
#include "Utilities.h"
#include <SFML/System.hpp>


int main() {

    GameEngine game("../config.txt");
    game.run();

    return 0;
}