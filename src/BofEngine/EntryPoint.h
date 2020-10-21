#pragma once


#include "Core.h"


extern Bof::Application* Bof::CreateApplication();



int main(int /*argc*/, char** /*argv*/)
{
    Bof::Application* app = Bof::CreateApplication();

    app->run();

    delete app;
}