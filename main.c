/**
   Just calls app_main. Makes easier for testing individual functions.
 */

int app_main(int ac, char** av);

int main(int ac, char** av)
{
  return app_main(ac, av);
}
