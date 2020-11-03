#include "vtkObjectFactory.h"
#include "vtkActor.h"

#include "vtkDataSetReader.h"
#include "vtkDataSetSurfaceFilter.h"

#include "vtkDistributedDataFilter.h"

#include "vtkOpenGLRenderWindow.h"
#include "vtkOverlayPass.h"
#include "vtkPieceScalars.h"
#include "vtkPKdTree.h"
#include "vtkPolyDataMapper.h"
#include"vtkDataSetMapper.h"
#include "vtkProperty.h"

#include "vtkOpenGLRenderer.h"
#include "vtkRenderPassCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSequencePass.h"
#include "vtkSmartPointer.h"                               
#include "vtkSocketController.h"
#include "vtkSphereSource.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkPVClientServerSynchronizedRenderers.h"
#include "vtkSynchronizedRenderWindows.h"

#include "vtkUnstructuredGrid.h"



#include"vtkUnstructuredGridReader.h"

#include <vtksys/CommandLineArguments.hxx>



int main(int argc, char *argv[])
{
	int port = 11111;

	vtkSmartPointer<vtkSocketController> contr =
		vtkSmartPointer<vtkSocketController>::New();
	contr->Initialize(&argc, &argv);


	if (!contr->ConnectTo(const_cast<char*>("localhost"), port))//connect to server
	{
		cout << "error" << endl;
		return 1;
	}
	cout << "success" << endl;

	vtkRenderWindow *rw = vtkRenderWindow::New();
	rw->SetWindowName("client");

	// enable alpha bit-planes.
	rw->AlphaBitPlanesOn();

	// use double bufferring.
	rw->DoubleBufferOn();

	// don't waste time swapping buffers unless needed.
	rw->SwapBuffersOff();

	vtkRenderer* renderer = vtkRenderer::New();
	rw->AddRenderer(renderer);

	vtkSynchronizedRenderWindows* syncWindows =
		vtkSynchronizedRenderWindows::New();
	syncWindows->SetRenderWindow(rw);
	syncWindows->SetParallelController(contr);
	syncWindows->SetIdentifier(2);
	syncWindows->SetRootProcessId(0);//client is 0


	vtkPVClientServerSynchronizedRenderers *syncRenderers = vtkPVClientServerSynchronizedRenderers::New();
	syncRenderers->SetRenderer(renderer);
	syncRenderers->SetParallelController(contr);
	syncRenderers->SetRootProcessId(0);
	syncRenderers->SetImageReductionFactor(1);


	// CLIENT
	vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
	iren->SetRenderWindow(rw);
	rw->SwapBuffersOn();
	iren->Start();

	contr->TriggerBreakRMIs();

	cout << "exit" << endl;
	return 0;
}

