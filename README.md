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
I'm also utilizing unreals multithreading classes ([FRunnable](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Core/HAL/FRunnable)) to split to work on all available threads of the cpu.

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
