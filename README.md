# Schloxel

Multithreaded Procedural Generation of Voxels via a Heightmap,

.vox file importer and modification of voxels. Build on top of UE5.

## Showcase 📹

**Voxel modification**

https://github.com/user-attachments/assets/fea56c04-8a06-49ea-92d9-911816f47987



**Procedural Generation using a Heightmap**

https://github.com/user-attachments/assets/eca5f606-8aa9-45e0-b005-affc4688e188

---

## Technical details (kinda) 👾
The Mesh is being generated using a [Greedy Meshing Algorithm](https://gedge.ca/blog/2014-08-17-greedy-voxel-meshing/). This way it's much quicker since less vertices are beign generated.

I'm also utilizing unreals multithreading classes ([FRunnable](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/HAL/FRunnable)) to split the work on all available threads of the cpu.
(Right now the landscape generation system is kinda basic tho since i just create a new thread for every chunk that is being created lol).

Atleast the optimazations for the importing of the .vox models is more advanced. Here i split the Model on it's Z axis and divide it by the number of threads my cpu has availible. This way when i modify the mesh by making a hole in it, the work for recalculation is being split. (You can see the visualization of the threads in the first video).

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
