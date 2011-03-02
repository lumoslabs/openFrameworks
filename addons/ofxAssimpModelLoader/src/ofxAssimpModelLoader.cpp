#include "ofxAssimpModelLoader.h"

#include "aiConfig.h"
#include <assert.h>

ofxAssimpModelLoader::ofxAssimpModelLoader(){

    lastAnimationTime = 0;
    currentAnimation = 0;
    animationTime = 0;
    numRotations = 0;
    rotAngle.clear();
    rotAxis.clear();
    scale = ofPoint(1, 1, 1);
	
	scene = NULL;
    normalizeScale = true;   
}

//------------------------------------------
void ofxAssimpModelLoader::loadModel(string modelName){
	
    
    // if we have a model loaded, unload the fucker.
    if(scene != NULL){
        aiReleaseImport(scene);
        scene = NULL;
        deleteGLResources();   
    }
    
    
    // Load our new path.
    filepath = ofToDataPath(modelName);
	
	//theo added - so we can have models and their textures in sub folders
	modelFolder = ofFileUtils::getEnclosingDirectoryFromPath(filepath);

    ofLog(OF_LOG_VERBOSE, "loading model %s", filepath.c_str());
    ofLog(OF_LOG_VERBOSE, "loading from folder %s", modelFolder.c_str());
    
    // only ever give us triangles.
    aiSetImportPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT );
    aiSetImportPropertyInteger(AI_CONFIG_PP_PTV_NORMALIZE, true);
    
    // aiProcess_FlipUVs is for VAR code. Not needed otherwise. Not sure why.
    scene = (aiScene*) aiImportFile(filepath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_OptimizeGraph | aiProcess_Triangulate | aiProcess_FlipUVs | 0 );
    
    if(scene){        
        ofLog(OF_LOG_VERBOSE, "initted scene with %i meshes & %i animations", scene->mNumMeshes, scene->mNumAnimations);

        getBoundingBoxWithMinVector(&scene_min, &scene_max);
        scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
        scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
        scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
        
        // optional normalized scaling
        normalizedScale = scene_max.x-scene_min.x;
        normalizedScale = MAX(scene_max.y - scene_min.y,normalizedScale);
        normalizedScale = MAX(scene_max.z - scene_min.z,normalizedScale);
        normalizedScale = 1.f / normalizedScale;
        normalizedScale *= ofGetWidth() / 2.0;
        
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
        
        loadGLResources();
        
        glPopClientAttrib();
        glPopAttrib();

        if(getAnimationCount())
            ofLog(OF_LOG_VERBOSE, "scene has animations");
        else {
            ofLog(OF_LOG_VERBOSE, "no animations");
            
        }
    }
}

//-------------------------------------------
void ofxAssimpModelLoader::loadGLResources(){

    ofLog(OF_LOG_VERBOSE, "loading gl resources");

    // create new mesh helpers for each mesh, will populate their data later.
    modelMeshes.resize(scene->mNumMeshes);
        
    // create OpenGL buffers and populate them based on each meshes pertinant info.
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i){
        ofLog(OF_LOG_VERBOSE, "loading mesh %u", i);
        // current mesh we are introspecting
        aiMesh* mesh = scene->mMeshes[i];
        
        // the current meshHelper we will be populating data into.
        ofxAssimpMeshHelper & meshHelper = modelMeshes[i];
        
        meshHelper.mesh = mesh;
        
        // set the mesh's default values.
        aiColor4D dcolor = aiColor4D(0.8f, 0.8f, 0.8f, 1.0f);
        meshHelper.diffuseColor = dcolor;

        aiColor4D scolor = aiColor4D(0.0f, 0.0f, 0.0f, 1.0f);
        meshHelper.specularColor = scolor;

        aiColor4D acolor = aiColor4D(0.2f, 0.2f, 0.2f, 1.0f);
        meshHelper.ambientColor = acolor;

        aiColor4D ecolor = aiColor4D(0.0f, 0.0f, 0.0f, 1.0f);
        meshHelper.emissiveColor = ecolor;

        // Handle material info
        aiMaterial* mtl = scene->mMaterials[mesh->mMaterialIndex];
        
        // Load Textures
        int texIndex = 0;
        aiString texPath;
        
        //meshHelper.texture = NULL;
        
        // TODO: handle other aiTextureTypes
        if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath)){
            // This is magic. Thanks Kyle.

            ofLog(OF_LOG_VERBOSE, "loading image from %s", texPath.data);
            string modelFolder = ofFileUtils::getEnclosingDirectoryFromPath(filepath);

			if(ofFileUtils::isAbsolute(texPath.data) && ofFileUtils::doesFileExist(texPath.data)) {
				meshHelper.texture.loadImage(texPath.data);
			}
			else {
				meshHelper.texture.loadImage(modelFolder + texPath.data);
			}
            meshHelper.texture.update();
            
            ofLog(OF_LOG_VERBOSE, "texture width: %f height %f", meshHelper.texture.getWidth(), meshHelper.texture.getHeight());
            
        }
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &dcolor))
            meshHelper.diffuseColor = dcolor;
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &scolor))
            meshHelper.specularColor = scolor;
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &acolor))
            meshHelper.ambientColor = acolor;
        
        if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &ecolor))
            meshHelper.emissiveColor = ecolor;
        
        // Culling
        unsigned int max = 1;
        int two_sided;
        if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
            meshHelper.twoSided = true;
        else
            meshHelper.twoSided = false;
        
        int usage;
        if(getAnimationCount()){
        	usage = GL_STREAM_DRAW;
        }else{
        	usage = GL_STATIC_DRAW;

        }
        meshHelper.vbo.setVertexData(&mesh->mVertices[0].x,3,mesh->mNumVertices,usage,sizeof(aiVector3D));
        if(mesh->HasVertexColors(0)){
        	meshHelper.vbo.setColorData(&mesh->mColors[0][0].r,mesh->mNumVertices,usage,sizeof(aiColor4D));
        }
        if(mesh->HasNormals()){
        	meshHelper.vbo.setNormalData(&mesh->mNormals[0].x,mesh->mNumVertices,usage,sizeof(aiVector3D));
        }
        if (mesh->HasTextureCoords(0)){
        	meshHelper.vbo.setTexCoordData(&mesh->mTextureCoords[0][0].x,mesh->mNumVertices,usage,sizeof(aiVector3D));
        }

        meshHelper.indices.resize(mesh->mNumFaces * 3);
        int i=0;
        for (unsigned int x = 0; x < mesh->mNumFaces; ++x){
			for (unsigned int a = 0; a < mesh->mFaces[x].mNumIndices; ++a){
				meshHelper.indices[i++]=mesh->mFaces[x].mIndices[a];
			}
		}

        meshHelper.vbo.setIndexData(&meshHelper.indices[0],meshHelper.indices.size(),GL_STATIC_DRAW);
    }
    ofLog(OF_LOG_VERBOSE, "finished loading gl resources");
}

//-------------------------------------------
void ofxAssimpModelLoader::deleteGLResources(){

    ofLog(OF_LOG_VERBOSE, "deleting gl resources");
    
    // clear out everything.
    modelMeshes.clear();
    
}

//-------------------------------------------
ofxAssimpModelLoader::~ofxAssimpModelLoader(){
}


//-------------------------------------------
void ofxAssimpModelLoader::getBoundingBoxWithMinVector(struct aiVector3D* min, struct aiVector3D* max)
{
	struct aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);
    
	min->x = min->y = min->z =  1e10f;
	max->x = max->y = max->z = -1e10f;
    
    this->getBoundingBoxForNode(scene->mRootNode, min, max, &trafo);
}

//-------------------------------------------
void ofxAssimpModelLoader::getBoundingBoxForNode(const struct aiNode* nd,  struct aiVector3D* min, struct aiVector3D* max, struct aiMatrix4x4* trafo)
{
	struct aiMatrix4x4 prev;
	unsigned int n = 0, t;
    
	prev = *trafo;
	aiMultiplyMatrix4(trafo,&nd->mTransformation);
    
	for (; n < nd->mNumMeshes; ++n){
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t){
        	struct aiVector3D tmp = mesh->mVertices[t];
			aiTransformVecByMatrix4(&tmp,trafo);
        
            
			min->x = MIN(min->x,tmp.x);
			min->y = MIN(min->y,tmp.y);
			min->z = MIN(min->z,tmp.z);
            
			max->x = MAX(max->x,tmp.x);
			max->y = MAX(max->y,tmp.y);
			max->z = MAX(max->z,tmp.z);
		}
	}
    
	for (n = 0; n < nd->mNumChildren; ++n){
		this->getBoundingBoxForNode(nd->mChildren[n], min, max, trafo);
	}
    
	*trafo = prev;
}

//-------------------------------------------
unsigned int ofxAssimpModelLoader::getAnimationCount(){
    if(scene)
        return scene->mNumAnimations; 
    else {
        ofLog(OF_LOG_WARNING, "No Model Loaded");
        return 0;
    }

}

//-------------------------------------------
void ofxAssimpModelLoader::setAnimation(int anim){
        currentAnimation = MIN(anim, (int)scene->mNumAnimations);
}

//-------------------------------------------
float ofxAssimpModelLoader::getDuration(int animation){
    const aiAnimation* anim = scene->mAnimations[animation];
    return anim->mDuration;
}


//-------------------------------------------
void ofxAssimpModelLoader::setNormalizedTime(float t){ // 0 - 1
    
    if(getAnimationCount())
    {

        const aiAnimation* anim = scene->mAnimations[currentAnimation];
        float realT = ofMap(t, 0.0, 1.0, 0.0, (float)anim->mDuration, false);

        setTime(realT);
    }
}

void ofxAssimpModelLoader::setTime(float t){ // 0 - 1
    if(getAnimationCount()){
        
        // only evaluate if we have a delta t.
        if(animationTime != t){
            animationTime = t;
            updateAnimation(currentAnimation, animationTime);
        }
    }
}


//-------------------------------------------
void ofxAssimpModelLoader::setPosition(float x, float y, float z){
    pos.x = x;
    pos.y = y;
    pos.z = z;
}

//-------------------------------------------
void ofxAssimpModelLoader::setScale(float x, float y, float z){
    scale.x = x;
    scale.y = y;
    scale.z = z;
}

//-------------------------------------------
void ofxAssimpModelLoader::setScaleNomalization(bool normalize)
{
    normalizeScale = normalize;
}


//-------------------------------------------
void ofxAssimpModelLoader::setRotation(int which, float angle, float rot_x, float rot_y, float rot_z){
    if(which + 1 > numRotations){
        int diff = 1 + (which - numRotations);
        for(int i = 0; i < diff; i++){
            rotAngle.push_back(0);
            rotAxis.push_back(ofPoint());
        }
        numRotations = rotAngle.size();
    }
    
    rotAngle[which]  = angle;
    rotAxis[which].x = rot_x;
    rotAxis[which].y = rot_y;
    rotAxis[which].z = rot_z;
}

//-------------------------------------------
void ofxAssimpModelLoader::updateAnimation(unsigned int animationIndex, float currentTime){
    
    const aiAnimation* mAnim = scene->mAnimations[animationIndex];
  
    // calculate the transformations for each animation channel
	for( unsigned int a = 0; a < mAnim->mNumChannels; a++)
	{
		const aiNodeAnim* channel = mAnim->mChannels[a];
                
        aiNode* targetNode = scene->mRootNode->FindNode(channel->mNodeName);

        // ******** Position *****
        aiVector3D presentPosition( 0, 0, 0);
        if( channel->mNumPositionKeys > 0)
        {
            // Look for present frame number. Search from last position if time is after the last time, else from beginning
            // Should be much quicker than always looking from start for the average use case.
            unsigned int frame = 0;// (currentTime >= lastAnimationTime) ? lastFramePositionIndex : 0;
            while( frame < channel->mNumPositionKeys - 1)
            {
                if( currentTime < channel->mPositionKeys[frame+1].mTime)
                    break;
                frame++;
            }
            
            // interpolate between this frame's value and next frame's value
            unsigned int nextFrame = (frame + 1) % channel->mNumPositionKeys;
            const aiVectorKey& key = channel->mPositionKeys[frame];
            const aiVectorKey& nextKey = channel->mPositionKeys[nextFrame];
            double diffTime = nextKey.mTime - key.mTime;
            if( diffTime < 0.0)
                diffTime += mAnim->mDuration;
            if( diffTime > 0)
            {
                float factor = float( (currentTime - key.mTime) / diffTime);
                presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
            } else
            {
                presentPosition = key.mValue;
            }
        }
        
        // ******** Rotation *********
        aiQuaternion presentRotation( 1, 0, 0, 0);
        if( channel->mNumRotationKeys > 0)
        {
            unsigned int frame = 0;//(currentTime >= lastAnimationTime) ? lastFrameRotationIndex : 0;
            while( frame < channel->mNumRotationKeys - 1)
            {
                if( currentTime < channel->mRotationKeys[frame+1].mTime)
                    break;
                frame++;
            }
            
            // interpolate between this frame's value and next frame's value
            unsigned int nextFrame = (frame + 1) % channel->mNumRotationKeys;
            const aiQuatKey& key = channel->mRotationKeys[frame];
            const aiQuatKey& nextKey = channel->mRotationKeys[nextFrame];
            double diffTime = nextKey.mTime - key.mTime;
            if( diffTime < 0.0)
                diffTime += mAnim->mDuration;
            if( diffTime > 0)
            {
                float factor = float( (currentTime - key.mTime) / diffTime);
                aiQuaternion::Interpolate( presentRotation, key.mValue, nextKey.mValue, factor);
            } else
            {
                presentRotation = key.mValue;
            }
        }
        
        // ******** Scaling **********
        aiVector3D presentScaling( 1, 1, 1);
        if( channel->mNumScalingKeys > 0)
        {
            unsigned int frame = 0;//(currentTime >= lastAnimationTime) ? lastFrameScaleIndex : 0;
            while( frame < channel->mNumScalingKeys - 1)
            {
                if( currentTime < channel->mScalingKeys[frame+1].mTime)
                    break;
                frame++;
            }
            
            // TODO: (thom) interpolation maybe? This time maybe even logarithmic, not linear
            presentScaling = channel->mScalingKeys[frame].mValue;
        }
        
        // build a transformation matrix from it
        //aiMatrix4x4& mat;// = mTransforms[a];
        aiMatrix4x4 mat = aiMatrix4x4( presentRotation.GetMatrix());
        mat.a1 *= presentScaling.x; mat.b1 *= presentScaling.x; mat.c1 *= presentScaling.x;
        mat.a2 *= presentScaling.y; mat.b2 *= presentScaling.y; mat.c2 *= presentScaling.y;
        mat.a3 *= presentScaling.z; mat.b3 *= presentScaling.z; mat.c3 *= presentScaling.z;
        mat.a4 = presentPosition.x; mat.b4 = presentPosition.y; mat.c4 = presentPosition.z;
        //mat.Transpose();
        
        targetNode->mTransformation = mat;

    }

    lastAnimationTime = currentTime;

}

//-------------------------------------------
void ofxAssimpModelLoader::updateGLResources(){
        
    // update mesh position for the animation
    for (unsigned int i = 0; i < modelMeshes.size(); ++i){

        // current mesh we are introspecting
        const aiMesh* mesh = modelMeshes[i].mesh;
        
        // calculate bone matrices
        std::vector<aiMatrix4x4> boneMatrices( mesh->mNumBones);
        for( size_t a = 0; a < mesh->mNumBones; ++a)
        {
            const aiBone* bone = mesh->mBones[a];
            
            // find the corresponding node by again looking recursively through the node hierarchy for the same name
            aiNode* node = scene->mRootNode->FindNode(bone->mName);
            
            // start with the mesh-to-bone matrix 
            boneMatrices[a] = bone->mOffsetMatrix;
            // and now append all node transformations down the parent chain until we're back at mesh coordinates again
            const aiNode* tempNode = node;
            while( tempNode)
            {
                // check your matrix multiplication order here!!!
                boneMatrices[a] = tempNode->mTransformation * boneMatrices[a];   
                // boneMatrices[a] = boneMatrices[a] * tempNode->mTransformation;
                tempNode = tempNode->mParent;
            }
        }
        
        // all using the results from the previous code snippet
        std::vector<aiVector3D> resultPos( mesh->mNumVertices); 
        std::vector<aiVector3D> resultNorm( mesh->mNumVertices);
        
        // loop through all vertex weights of all bones
        for( size_t a = 0; a < mesh->mNumBones; ++a)
        {
            const aiBone* bone = mesh->mBones[a];
            const aiMatrix4x4& posTrafo = boneMatrices[a];
            
            // 3x3 matrix, contains the bone matrix without the translation, only with rotation and possibly scaling
            aiMatrix3x3 normTrafo = aiMatrix3x3( posTrafo); 
            for( size_t b = 0; b < bone->mNumWeights; ++b)
            {
                const aiVertexWeight& weight = bone->mWeights[b];
                
                size_t vertexId = weight.mVertexId; 
                const aiVector3D& srcPos = mesh->mVertices[vertexId];
                const aiVector3D& srcNorm = mesh->mNormals[vertexId];
                
                resultPos[vertexId] += weight.mWeight * (posTrafo * srcPos);
                resultNorm[vertexId] += weight.mWeight * (normTrafo * srcNorm);
            }
        }
                
        // now upload the result position and normal along with the other vertex attributes into a dynamic vertex buffer, VBO or whatever
        

        modelMeshes[i].vbo.updateVertexData(&resultPos[0].x,mesh->mNumVertices);
        if(mesh->HasVertexColors(0)){
        	 modelMeshes[i].vbo.updateColorData(&mesh->mColors[0][0].r,mesh->mNumVertices);
        }
        if(mesh->HasTextureCoords(0)){
        	 modelMeshes[i].vbo.updateTexCoordData(&mesh->mTextureCoords[0][0].x,mesh->mNumVertices);
        }
        if(mesh->HasNormals()){
        	 modelMeshes[i].vbo.updateNormalData(&mesh->mNormals[0].x,mesh->mNumVertices);
        }
    }
}


void ofxAssimpModelLoader::draw()
{
    if(scene){
        
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
        
        glEnable(GL_NORMALIZE);
        
        glPushMatrix();
            
        glTranslatef(pos.x, pos.y, pos.z);

        glRotatef(180, 0, 0, 1);
        glTranslated(-scene_center.x, -scene_center.y, scene_center.z);    

        if(normalizeScale)
        {
            glScaled(normalizedScale , normalizedScale, normalizedScale);
        }
            
        for(int i = 0; i < numRotations; i++){
            glRotatef(rotAngle[i], rotAxis[i].x, rotAxis[i].y, rotAxis[i].z);
        }
        
        glScalef(scale.x, scale.y, scale.z);
        
        if(getAnimationCount())
        {
            updateGLResources();
        }
        
		for(int i = 0; i < (int)modelMeshes.size(); i++){
			ofxAssimpMeshHelper & meshHelper = modelMeshes.at(i);

			// Texture Binding
			if(meshHelper.texture.bAllocated()){
				meshHelper.texture.getTextureReference().bind();
			}

			// Material colors and properties
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &meshHelper.diffuseColor.r);

			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &meshHelper.specularColor.r);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &meshHelper.ambientColor.r);

			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &meshHelper.emissiveColor.r);


			// Culling
			if(meshHelper.twoSided)
				glEnable(GL_CULL_FACE);
			else
				glDisable(GL_CULL_FACE);

		    meshHelper.vbo.drawElements(GL_TRIANGLES,meshHelper.indices.size());

			// Texture Binding
			if(meshHelper.texture.bAllocated()){
				meshHelper.texture.getTextureReference().unbind();
			}
		}
            
        glPopMatrix();
        
        glPopClientAttrib();
        glPopAttrib();
    }
}
