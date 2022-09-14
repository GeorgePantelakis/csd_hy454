#include "../Include/app.h"

int main() {

    app::MainApp *app = new app::MainApp();

    app->Main();
    
    delete app;

    return 0;
}
