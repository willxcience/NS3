#include <Python.h>
#include <iostream>

using namespace std;
int
main(int argc, char *argv[])
{
  Py_SetProgramName(argv[0]);  /* optional but recommended */
  Py_Initialize();

        if (!Py_IsInitialized())
        {
                printf("init error\n");
                return -1;
        }


    int res;
    PyObject *pModule,*pFunc;
    PyObject *pArgs, *pValue;

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
    
    /* import */
    pModule = PyImport_ImportModule("great_module");

    if (!pModule) {
       printf("Cant open python file!\n");
       return -2;
    }

    /* great_module.great_function */
    pFunc = PyObject_GetAttrString(pModule, "asd"); 
    
    cout << "loaded" << endl;
    /* build args */
    pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs,0,PyInt_FromLong(100));
      
    /* call */
    pValue = PyObject_CallObject(pFunc, pArgs);
    
    res = PyInt_AsLong(pValue);
	
  cout << res << endl;


  Py_Finalize();
  return 0;
}

