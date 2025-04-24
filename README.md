# Schloxel

### Multithreaded Procedural Generation of Voxels via a Heightmap,

### .vox file importer and modification of voxels. Build on top of UE5.

## Showcase 📹

** Voxel modification

https://github.com/user-attachments/assets/fea56c04-8a06-49ea-92d9-911816f47987

---

** Procedural Generation using a Heightmap

https://github.com/user-attachments/assets/eca5f606-8aa9-45e0-b005-affc4688e188

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
