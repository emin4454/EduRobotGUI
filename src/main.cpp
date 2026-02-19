#include <thread>
#include <chrono>
#include <iostream>
#include "stdio.h"
#include "display_thread.h"
#include "edu_robot.h"
#include "globals.h"
#include "base64.h"
#include "fstream"
#include <fstream>
#include "rasa_client.h"
#include "tts_client.h"
#include "translate.h"
#include "env_utils.h"
#include <vector>
int main()
{
  env::load_dotenv(".env");
  EduRobot eduRobot;
}
