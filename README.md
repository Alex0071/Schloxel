# Schloxel

Multithreaded Procedural Generation of Voxels via a Heightmap

.vox file importer and modification of voxels. Build on top of UE5.

## Showcase 📹

**Voxel modification**

https://github.com/user-attachments/assets/fea56c04-8a06-49ea-92d9-911816f47987



**Procedural Generation using a Heightmap**


https://github.com/user-attachments/assets/0d818cd3-b55d-4939-a189-c7c24608c127


---

## Technical details (kinda) 👾
The Mesh is being generated using a [Greedy Meshing Algorithm](https://gedge.ca/blog/2014-08-17-greedy-voxel-meshing/). This way it's much quicker since fewer vertices are being generated.

I'm also utilizing unreals multithreading classes ([FRunnable](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/HAL/FRunnable)) to split the work on all available threads of the cpu.
(Right now the landscape generation system is kinda basic tho since i just create a new thread for every chunk that is being created lol).

At least the optimizations for the importing of the .vox models is more advanced. Here i split the Model on its Z axis and divide it by the number of threads my cpu has available. This way when i modify the mesh by making a hole in it, the work for recalculation is being split. (You can see the visualization of the threads in the first video).

At the end, i am using the lambda function `AsyncTask` to actually apply the mesh. Why? See, right now the mesh calculation is happening on a background thread. But you need to be on the `GameThread` to apply it. (And you need to be async so you don't acces no invalid memory or something).
```c++
void AVoxModel::ApplyMesh()
{
	AsyncTask(ENamedThreads::GameThread, [this]()
	{
		while (!MeshDataQueue.IsEmpty())
		{
			TQueue<FVoxMeshData>::FElementType data;
			if (MeshDataQueue.Dequeue(data))
			{...
```
