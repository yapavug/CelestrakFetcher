include("D:/QtApp/CelestrakFetcher/build/6_9_1_mingw64_shared-Debug/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/CelestrakFetcher-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE D:/QtApp/CelestrakFetcher/build/6_9_1_mingw64_shared-Debug/CelestrakFetcher.exe
    GENERATE_QT_CONF
)
