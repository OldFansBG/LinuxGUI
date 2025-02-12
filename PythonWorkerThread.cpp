#include "PythonWorkerThread.h"
#include "SecondWindow.h"
#include <Python.h>
#include <iostream>
#include <stdexcept>
#include <string>

PythonWorkerThread::PythonWorkerThread(SecondWindow* handler, const wxString& isoPath)
    : wxThread(wxTHREAD_JOINABLE),
      m_handler(handler),
      m_isoPath(isoPath) {
}

PythonWorkerThread::~PythonWorkerThread() {
    // DO NOT CALL Py_FinalizeEx() here
    // Python should be finalized in the main thread, not in the worker thread
}

wxThread::ExitCode PythonWorkerThread::Entry() {
    // ========================================================================
    // 1. Acquire the Global Interpreter Lock (GIL)
    // ========================================================================
    PyGILState_STATE gstate = PyGILState_Ensure();

    try {
        // Debug: Verify Python environment
        PyRun_SimpleString("import sys; print('sys.path:', sys.path)");
        PyRun_SimpleString("import _socket; print('_socket module loaded')");

        // ====================================================================
        // 2. Import Docker Module
        // ====================================================================
        PyObject* dockerModule = PyImport_ImportModule("docker");
        if (!dockerModule) {
            SendLogUpdate("Failed to import docker module");
            HandlePythonError();
            PyGILState_Release(gstate);
            return (ExitCode)1;
        }
        Py_DECREF(dockerModule);

        // ====================================================================
        // 3. Generate and Execute Python Code
        // ====================================================================
        const char* pythonCode = GeneratePythonCode();
        bool success = ExecutePythonCode(pythonCode);
        SendLogUpdate(success ? "Python task completed successfully." : "Python task failed.");

        // ====================================================================
        // 4. Send Completion Event
        // ====================================================================
        wxCommandEvent event(PYTHON_TASK_COMPLETED);
        event.SetInt(success ? 1 : 0);
        if (m_handler) {
            wxQueueEvent(m_handler, event.Clone());
        }

    } catch (const std::exception& e) {
        SendLogUpdate(wxString::Format("Error: %s", e.what()));
        
        wxCommandEvent event(PYTHON_TASK_COMPLETED);
        event.SetInt(0);
        event.SetString(e.what());
        if (m_handler) {
            wxQueueEvent(m_handler, event.Clone());
        }
    }

    // ========================================================================
    // 5. Release the Global Interpreter Lock (GIL)
    // ========================================================================
    PyGILState_Release(gstate);
    return (ExitCode)0;
}

void PythonWorkerThread::SendLogUpdate(const wxString& message) {
    if (m_handler) {
        wxCommandEvent event(PYTHON_LOG_UPDATE);
        event.SetString(message);
        wxQueueEvent(m_handler, event.Clone());
    }
}

bool PythonWorkerThread::ExecutePythonCode(const char* code) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    bool success = false;

    try {
        // Import the __main__ module and get its dictionary
        PyObject* mainModule = PyImport_AddModule("__main__");
        if (!mainModule) {
            throw std::runtime_error("Failed to get __main__ module.");
        }

        PyObject* globalDict = PyModule_GetDict(mainModule);
        if (!globalDict) {
            throw std::runtime_error("Failed to get global dictionary.");
        }

        // Execute the Python code
        PyObject* result = PyRun_String(code, Py_file_input, globalDict, globalDict);
        if (!result) {
            HandlePythonError();
            throw std::runtime_error("Failed to execute Python code.");
        }

        success = true;
        Py_XDECREF(result);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        success = false;
    }

    PyGILState_Release(gstate);
    return success;
}

void PythonWorkerThread::HandlePythonError() {
    if (PyErr_Occurred()) {
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);
        PyErr_NormalizeException(&type, &value, &traceback);

        PyObject* tbModule = PyImport_ImportModule("traceback");
        if (tbModule) {
            PyObject* formatFunc = PyObject_GetAttrString(tbModule, "format_exception");
            if (formatFunc) {
                PyObject* tbList = PyObject_CallFunctionObjArgs(formatFunc, type, value, traceback, NULL);
                if (tbList) {
                    PyObject* tbStr = PyUnicode_Join(PyUnicode_FromString(""), tbList);
                    if (tbStr) {
                        const char* fullError = PyUnicode_AsUTF8(tbStr);
                        SendLogUpdate(wxString(fullError));
                        std::cerr << "Python Error Details:\n" << fullError << std::endl;
                        Py_XDECREF(tbStr);
                    }
                    Py_XDECREF(tbList);
                }
                Py_XDECREF(formatFunc);
            }
            Py_XDECREF(tbModule);
        }

        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
    }
}

const char* PythonWorkerThread::GeneratePythonCode() {
    m_generatedCode = wxString::Format(
        R"PYCODE(
import docker
import os
import time
import traceback
import sys
import tarfile
import io

def copy_iso_to_container(container, host_path, container_dest_path):
    try:
        print(f"[PROGRESS] 20%% - Starting ISO copy: {os.path.basename(host_path)}")
        if not os.path.exists(host_path):
            raise FileNotFoundError(f"ISO file not found: {host_path}")
        
        tarstream = io.BytesIO()
        with tarfile.open(fileobj=tarstream, mode="w") as tar:
            tar.add(host_path, arcname=os.path.basename(container_dest_path))
        tarstream.seek(0)
        
        dest_dir = os.path.dirname(container_dest_path) or "/"
        container.put_archive(dest_dir, tarstream)
        print("[PROGRESS] 30%% - ISO copy completed")

    except Exception as e:
        print(f"[COPY ERROR] {str(e)}")
        raise

def validate_scripts(base_dir):
    required_scripts = ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]
    for script in required_scripts:
        script_path = os.path.join(base_dir, script)
        if not os.path.exists(script_path):
            raise FileNotFoundError(f"Missing script: {script_path}")
        if not os.access(script_path, os.X_OK):
            raise PermissionError(f"Script not executable: {script_path}")

def main():
    try:
        print("[PROGRESS] 10%% - Initializing Docker client")
        client = docker.from_env()
        client.ping()

        print("[PROGRESS] 15%% - Checking existing containers")
        try:
            old_container = client.containers.get("my_unique_container")
            print("[PROGRESS] 16%% - Removing old container")
            old_container.remove(force=True)
            time.sleep(2)
        except docker.errors.NotFound:
            pass

        print("[PROGRESS] 20%% - Creating new container")
        container = client.containers.run(
            "ubuntu:latest",
            command="sleep infinity",
            name="my_unique_container",
            detach=True,
            privileged=True
        )
        container.reload()
        if container.status != "running":
            raise RuntimeError(f"Container failed to start. Status: {container.status}")

        print("[PROGRESS] 25%% - Validating scripts")
        base_dir = r"%s"
        validate_scripts(base_dir)

        iso_path = r"%s"
        copy_iso_to_container(container, iso_path, "/base.iso")

        print("[PROGRESS] 40%% - Copying scripts to container")
        for script in ["setup_output.sh", "setup_chroot.sh", "create_iso.sh"]:
            host_path = os.path.join(base_dir, script)
            
            tarstream = io.BytesIO()
            with tarfile.open(fileobj=tarstream, mode="w") as tar:
                tar.add(host_path, arcname=script)
            tarstream.seek(0)
            container.put_archive("/", tarstream)

        print("[PROGRESS] 50%% - Setting permissions")
        exit_code, output = container.exec_run("chmod +x /setup_output.sh /setup_chroot.sh /create_iso.sh")
        if exit_code != 0:
            raise RuntimeError(f"Permission setup failed: {output.decode()}")

        print("[PROGRESS] 60%% - Running setup_output.sh")
        exit_code, output = container.exec_run("/setup_output.sh")
        if exit_code != 0:
            raise RuntimeError(f"setup_output.sh failed: {output.decode()}")

        print("[PROGRESS] 70%% - Running setup_chroot.sh")
        exit_code, output_gen = container.exec_run("/setup_chroot.sh", stream=True)
        ready_signal = False
        for line in output_gen:
            line_str = line.decode().strip()
            print(f"[CHROOT] {line_str}")
            if "Ready" in line_str:
                ready_signal = True
                break
        if not ready_signal or exit_code != 0:
            raise RuntimeError("setup_chroot.sh did not complete successfully")

        print("[PROGRESS] 80%% - Running create_iso.sh")
        exit_code, output = container.exec_run("/create_iso.sh", timeout=300)
        if exit_code != 0:
            raise RuntimeError(f"create_iso.sh failed: {output.decode()}")

        print("[PROGRESS] 100%% - All operations completed successfully")

    except Exception as e:
        print(f"[FATAL ERROR] {str(e)}")
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
        )PYCODE",
        "I:\\Files\\Desktop\\LinuxGUI", // Base directory
        m_isoPath // ISO path
    ).ToStdString();

    return m_generatedCode.c_str();
}