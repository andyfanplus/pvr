#include "vtkSmartPointer.h"

#include "vtkMPIController.h"
#include "vtkSocketController.h"

#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkActor.h"
#include "vtkVolume.h"
#include "vtkSynchronizedRenderWindows.h"
#include "vtkPVClientServerSynchronizedRenderers.h"
#include "vtkSynchronizedRenderers.h"
#include "vtkIceTSynchronizedRenderers.h"
#include "vtkProperty.h"
#include "vtkPartitionOrderingInterface.h"
#include "vtkPKdTree.h"

#include "vtkPolyDataMapper.h"
#include"vtkDataSetMapper.h"
#include "vtkProjectedTetrahedraMapper.h"

#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkVolumeProperty.h"

#include "vtkDistributedDataFilter.h"
#include "vtkDataSetTriangleFilter.h"

#include "vtkDataSet.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetReader.h"
#include "vtkXMLMultiBlockDataReader.h"


#include<bits/stdc++.h>
using namespace  std;



void surfacerendering(vtkDataSet *ds, vtkRenderer *renderer)
{
	vtkDataSetMapper *mapper=vtkDataSetMapper::New();
	mapper->SetInputData(ds);

	vtkActor *actor = vtkActor::New();
	actor->SetMapper(mapper);
	vtkProperty *p = actor->GetProperty();
	//p->SetOpacity(0.3);

	renderer->AddActor(actor);
}

void volumerendering(vtkDataSet *ds, vtkRenderer *renderer)
{

	vtkDataSetTriangleFilter *filter = vtkDataSetTriangleFilter::New();
	filter->SetInputData(ds);
	filter->Update();

	auto ug = filter->GetOutput();

	auto range = ug->GetScalarRange();

	vtkProjectedTetrahedraMapper *mapper = vtkProjectedTetrahedraMapper::New();
	mapper->SetInputData(ug);
	vtkColorTransferFunction *color = vtkColorTransferFunction::New();
	vtkPiecewiseFunction *opacity = vtkPiecewiseFunction::New();

	color->AddRGBPoint(range[0], 1, 0, 1);
	color->AddRGBPoint(range[1], 0, 1, 1);
	opacity->AddPoint(range[0], 0);
	opacity->AddPoint(range[1], 1);

	vtkVolumeProperty *property = vtkVolumeProperty::New();
	property->SetColor(color);
	property->SetScalarOpacity(opacity);

	vtkVolume *volume = vtkVolume::New();
	volume->SetMapper(mapper);
	volume->SetProperty(property);

	renderer->AddVolume(volume);
}

int main(int argc, char **argv)
{
	vtkSmartPointer<vtkMPIController> mpiController = vtkSmartPointer<vtkMPIController>::New();
	mpiController->Initialize(&argc, &argv, 0);
	int my_id = mpiController->GetLocalProcessId();
	int num_procs = mpiController->GetNumberOfProcesses();

	vtkSocketController *socketController = vtkSocketController::New();

	int isConnected = 0;
	if (my_id == 0)
	{
		int port = 11111;
		socketController->Initialize(&argc, &argv);
		cout << "Waiting for client on " << port << endl;
		socketController->WaitForConnection(port); //�����˿�

		isConnected = 1;
		mpiController->Broadcast(&isConnected, 1, 0);
	}
	else
	{
		mpiController->Broadcast(&isConnected, 1, 0);
	}

	if (isConnected == 0)
	{
		mpiController->Finalize();
		cout << "connect failed" << endl;
		return 0;
	}
	else
	{
		if (my_id == 0)
			cout << "connect established" << endl; //connect succuss
	}

	vtkDataSet *input = nullptr;
	vtkUnstructuredGrid *output = vtkUnstructuredGrid::New();

	if (my_id == 0)
	{
		vtkDataSetReader *reader = vtkDataSetReader::New();
		reader->SetFileName("comb.vtk");
		reader->Update();
		input = reader->GetOutput();
	}
	else
	{
		input = static_cast<vtkDataSet *>(output);
	}

	vtkDistributedDataFilter *d3 = vtkDistributedDataFilter::New();
	d3->SetInputData(input);
	d3->SetController(mpiController);
	d3->SetBoundaryModeToSplitBoundaryCells();
	d3->UseMinimalMemoryOff();
	d3->Update();

	output = vtkUnstructuredGrid::SafeDownCast(d3->GetOutput());
	cout << output->GetNumberOfCells() << endl;

	vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
	vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
	renWin->AddRenderer(renderer);
	renWin->SetPosition(my_id * 410, 0);
	renWin->SetSize(400, 400);
	renWin->SetWindowName(to_string(my_id).c_str());

	//visualization pipeline
	volumerendering(output, renderer);
	//surfacerendering(output, renderer);

	vtkSmartPointer<vtkSynchronizedRenderWindows> mpiSyncWindows =
		vtkSmartPointer<vtkSynchronizedRenderWindows>::New();
	mpiSyncWindows->SetRenderWindow(renWin);
	mpiSyncWindows->SetParallelController(mpiController);
	mpiSyncWindows->SetIdentifier(231);

	vtkSmartPointer<vtkIceTSynchronizedRenderers> mpiSyncRenderers =
		vtkSmartPointer<vtkIceTSynchronizedRenderers>::New();
	mpiSyncRenderers->SetRenderer(renderer);
	mpiSyncRenderers->SetParallelController(mpiController);
	mpiSyncRenderers->ParallelRenderingOn();
	mpiSyncRenderers->SetIdentifier(231);
	mpiSyncRenderers->SetUseOrderedCompositing(true);
	vtkPartitionOrderingInterface *partition = vtkPartitionOrderingInterface::New();
	partition->SetImplementation(d3->GetKdtree());
	mpiSyncRenderers->SetPartitionOrdering(partition);

	if (my_id == 0)
	{
		vtkSynchronizedRenderWindows *csSyncWindows =
			vtkSynchronizedRenderWindows::New();
		csSyncWindows->SetRenderWindow(renWin);
		csSyncWindows->SetParallelController(socketController);
		csSyncWindows->SetIdentifier(2);
		csSyncWindows->SetRootProcessId(1); //server is 1

		vtkPVClientServerSynchronizedRenderers *csSyncRenderers = vtkPVClientServerSynchronizedRenderers::New();
		csSyncRenderers->SetRenderer(renderer);
		csSyncRenderers->SetParallelController(socketController);
		csSyncRenderers->SetRootProcessId(1); //server is 1
		csSyncRenderers->SetImageReductionFactor(1);

		csSyncRenderers->SetCaptureDelegate(mpiSyncRenderers);
		mpiSyncRenderers->AutomaticEventHandlingOff();
	}

	//renWin->OffScreenRenderingOn();// server offscreen rendering
	if (my_id == 0)
	{
		socketController->ProcessRMIs();
	}
	mpiController->ProcessRMIs();

	if (my_id == 0)
		cout << "exit" << endl;
	return 0;
}
