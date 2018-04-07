#include "tracer.h"

Vec3f rotateX(Vec3f p, double theta){
    Vec3f ans;
	ans.y = cos(theta)*p.y - sin(theta)*p.z;
	ans.z= cos(theta)*p.z + sin(theta)*p.y;
	ans.x = p.x;
	return ans;
}

Vec3f rotateY(Vec3f p, double theta){
	Vec3f ans;
	ans.x = cos(theta)*p.x + sin(theta)*p.z;
	ans.z = cos(theta)*p.z - sin(theta)*p.x;
	ans.y = p.y;
	return ans;
}

Vec3f rotateZ(Vec3f p, double theta){
	Vec3f ans;
	ans.x = cos(theta)*p.x - sin(theta)*p.y;
	ans.y = cos(theta)*p.y + sin(theta)*p.x;
	ans.z = p.z;
	return ans;
}

inline bool isObtuse(Vec3f a, Vec3f v){
    return ((a.x * v.x + a.y * v.y + a.z * v.z) < 0);
}

Vec3f unit_cross_product(Vec3f v1, Vec3f v2){
    Vec3f ans;
    ans.x = v1.y*v2.z - v1.z*v2.y;
    ans.y = v1.z*v2.x - v1.x*v2.z;
    ans.z = v1.x*v2.y - v1.y*v2.x;
    
    ans.normalize();
    return ans;
}

mesh::mesh(string objfile, color col, Vec3f cen, double scale_factor){
    ifstream objstream(objfile.c_str());
    string line;
    num_vertices = 0;
    num_faces = 0;
    centre = cen;
    while(getline(objstream,line)){
        if(line.c_str()[0] == '#')
            continue;
        
        if(line.c_str()[0] == 'v' && line.c_str()[1] == ' '){
            stringstream s(line);
            Vec3f vertex;
            char tmp;
            s >> tmp >> vertex.x >> vertex.y >> vertex.z;
            vertex.x *= scale_factor;
            vertex.y *= scale_factor;
            vertex.z *= scale_factor;
            maxX = max(maxX,vertex.x);
            maxY = max(maxY,vertex.y);
            maxZ = max(maxZ,vertex.z);
            minX = min(minX,vertex.x);
            minY = min(minY,vertex.y);
            minZ = min(minZ,vertex.z);
            vertices.push_back(vertex);
            
        }

        if(line.c_str()[0] == 'f' && line.c_str()[1] == ' '){
            stringstream s(line);
            int a, b, c;
            char tmp;
            s >> tmp >> a >> b >> c;
            meshPlane tmpface(a-1,b-1,c-1,col.R,col.G,col.B);
            tmpface.n = unit_cross_product(vertices[a-1]-vertices[b-1],vertices[b-1]-vertices[c-1]);
            faces.push_back(tmpface);
        }

    }

    objstream.close();
    
    Vec3f cen1((maxX + minX)/2,(maxY + minY)/2,(maxZ + minZ)/2);
    cout << "centre :";
    cen1.print();
    Vec3f disp = centre - cen1;
    disp.print();
    cout << "Min Z" << maxZ << endl;
    for(int i = 0; i < vertices.size();i++){
        vertices[i] = vertices[i] + disp;
        // cout << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << endl;
    }

    num_faces = faces.size();
    num_vertices = vertices.size();

}

mesh::mesh(mesh &originalMesh,double &thetaX, double &thetaY, double &thetaZ){
    centre = originalMesh.centre;
    for (int i = 0; i < originalMesh.num_vertices; ++i){
        this->vertices.push_back(rotateX(rotateY(rotateZ(\
        originalMesh.vertices[i] - originalMesh.centre,thetaZ),thetaY),thetaX) + originalMesh.centre);
    }

    for (int i = 0; i < originalMesh.num_faces; ++i){
        faces.push_back(originalMesh.faces[i]);
        Vec3f a = vertices[faces[i].a];
        Vec3f b = vertices[faces[i].b];
        Vec3f c = vertices[faces[i].c];
        faces[i].n = unit_cross_product(a-b,b-c);
    }
    num_faces = faces.size();
    num_vertices = vertices.size();
}


bool isOnFace(Vec3f &pt, meshPlane &face, mesh &parent_mesh){
    Vec3f v0 = parent_mesh.vertices[face.c] - parent_mesh.vertices[face.a];
    Vec3f v1 = parent_mesh.vertices[face.b] - parent_mesh.vertices[face.a];
    Vec3f v2 = pt - parent_mesh.vertices[face.a];

    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);

    double invDenom = 1/(dot11*dot00 -dot01*dot01);

    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u > -1e-5) and (v > -1e-5) and (u + v < 1+1e-5);
}

bool isOnFace(Vec3f &pt, Wall &w){
    Vec3f P1_P4 = w.a - w.d;
    Vec3f P3_P4 = w.c - w.d;
    Vec3f TWO_P_C = pt*2 - w.a - w.c;
    return (P3_P4.dot(TWO_P_C - P3_P4) <= 1e-5 && P3_P4.dot(TWO_P_C + P3_P4) >= -1e-5) &&
        (P1_P4.dot(TWO_P_C - P1_P4) <= 1e-5 && P1_P4.dot(TWO_P_C + P1_P4) >= -1e-5);
}

bool getIntersection(Vec3f p1, Vec3f p2, Vec3f &pt, meshPlane &face, mesh &parent_mesh){
    double t1 = (p1 - p2).dot(face.n);

    if(t1 == 0)
        return false;
    
    Vec3f arbpt = (parent_mesh.vertices[face.a] + parent_mesh.vertices[face.b] + parent_mesh.vertices[face.c])/3.0;
    double t2 = (arbpt - p1).dot(face.n);
    double t = t2/t1;
    if(t < 1)
        return false;
    
    pt = p1 + (p2 - p1)*t;
    if(isOnFace(pt,face,parent_mesh))
        return true;
    return false;
}

bool getIntersection(Vec3f p1, Vec3f p2, Vec3f &pt, Wall &w){
    double t1 = (p1 - p2).dot(w.n);

    if(t1 == 0)
        return false;
    
    Vec3f arbpt = w.a;
    double t2 = (arbpt - p1).dot(w.n);
    double t = t2/t1;
    if(t < 1)
        return false;
    pt = p1 + (p2 - p1)*t;

    if(isOnFace(pt,w))
        return true;
    return false;
}

void init_bufs(){
    std::memset(raster, 0, sizeof(raster));
    for(int i = 0; i <= WALL_SIDE; i++){
        for(int j = 0; j <= WALL_SIDE; j++){
            zBuffer[i][j] = -1e8;
        }
    }
}

void backface_culling(mesh &mymesh, Vec3f cam, bool visible_faces[]){
    for(int i = 0; i < mymesh.num_faces; i++){
        Vec3f shootVec = mymesh.vertices[mymesh.faces[i].a] - cam;

        if(isObtuse(shootVec, mymesh.faces[i].n)){
            visible_faces[i] = true;
            // cout << "Visible" << endl;
        }
        else
            visible_faces[i] = false;
    }
}

void NormaliseV(){
    pair<int,int> key;
    map<pair<int,int>,double>::const_iterator it;

    for(int i = 0; i <= WALL_SIDE; i++){
        for(int j = 0; j <= WALL_SIDE; j++){
           key.first = (int)(hsvraster[i][j][0] * 1000) ;
            key.second = (int)(hsvraster[i][j][1] * 100000);
            it = hsImap.find(key);
            if( it == hsImap.end()){
                hsImap[key] = hsvraster[i][j][2];
            }
            else if(it->second < hsvraster[i][j][2]){
                hsImap[key] = hsvraster[i][j][2];
            } 
        }
    }
    double fR,fG,fB;
    for(int i = 0; i <= WALL_SIDE; i++){
        for(int j = 0; j <= WALL_SIDE; j++){
            key.first = (int)(hsvraster[i][j][0] * 1000) ;
            key.second = (int)(hsvraster[i][j][1] * 100000);
            it = hsImap.find(key);
            
            hsvraster[i][j][2] = hsvraster[i][j][2]/it->second;

            HSVtoRGB(fR,fG,fB,hsvraster[i][j][0],hsvraster[i][j][1],hsvraster[i][j][2]);
            raster[i][j][0] = ((int)(fR * 255));
            raster[i][j][1] = ((int)(fG * 255));
            raster[i][j][2] = ((int)(fB * 255));
        }
    }
}

double illuminatePoint(Vec3f &pt, Vec3f &cam, Vec3f &light,int faceInd, mesh &parent_mesh){
    double ambient = Ka * Ia;
    Vec3f n = parent_mesh.faces[faceInd].n;
    Vec3f l = (light - pt).normalize();
    Vec3f v = (cam - pt).normalize();
    double dist = (cam - pt).length();
    double fatt = min(1.0,1/(c1 + c2 * dist + c3 *dist*dist));
    double diffusion = Kd * Ip * fatt * l.dot(n);
    
    if(diffusion < 0)
        diffusion = 0;
    //     cout << "BAMM!" << endl;
    double specular = Ks * Ip * fatt * pow(((2 * l.dot(n) * v.dot(n)) - l.dot(v)),specN);
    if(specular < 0)
    //     cout << "BAMM!!" << endl;
        specular = 0;
    double ans =  ambient + diffusion + specular;

    return ans;

}

double illuminatePoint(Vec3f &pt, Vec3f &cam, Vec3f &light,int faceInd, int meshIndex, vector<mesh> &meshes){
    double ambient = Ka * Ia;
    Vec3f n = meshes[meshIndex].faces[faceInd].n;
    Vec3f l = (light - pt).normalize();
    Vec3f v = (cam - pt).normalize();
    double dist = (cam - pt).length();
    double fatt = min(1.0,1/(c1 + c2 * dist + c3 *dist*dist));
    double diffusion = Kd * Ip * fatt * l.dot(n);

    if(diffusion < 0)
        diffusion = 0;
    double specular = Ks * Ip * fatt * pow(((2 * l.dot(n) * v.dot(n)) - l.dot(v)),specN);
    if(specular < 0)
        specular = 0;
    double ans;

    if(!CrossShadows){
        ans =  ambient + diffusion + specular;
        return ans;
    }

    int f = 1;
    for(int i = 0; i < meshes.size(); i++){
        if(i != meshIndex and ShadowCheckIntersection(light,pt,meshes[i])){
            f = 0;
            break;
        }
    }

    ans = ambient + f * (diffusion + specular);
    return ans;
    
}


/**
 * Function called by illuminatePoint function
 * Checks any of the faces except the face the point is on
 * obstructs light
 */

bool ShadowCheckIntersection(Vec3f &light, Vec3f pt, int faceInd,mesh &parent_mesh){
    double t1,t2,t;
    for(int i = 0; i < parent_mesh.num_faces; i++){
        if(i != faceInd){
            t1 = (light-pt).dot(parent_mesh.faces[i].n);
            t2 = (parent_mesh.vertices[parent_mesh.faces[i].a] - pt).dot(parent_mesh.faces[i].n);

            if(t1 == 0)
                continue;
            t = t2/t1;
            Vec3f newpt = pt + (light-pt)*t;
            if(t > 0 and t < 1 and isOnFace(newpt,parent_mesh.faces[i],parent_mesh)){
                return true;
            }   
        }
    }
    return false;
}


bool ShadowCheckIntersection(Vec3f &light, Vec3f pt, mesh &parent_mesh){
    for(int i = 0; i < parent_mesh.num_faces; i++){
        if(ShadowCheckIntersection(light,pt,-1,parent_mesh))
            return true;
    }
    return false;
}
/**
 * To illuminate a point on a wall, when a single mesh is present
 * Finds if the light intersects the mesh on its way from source 
 * to that point and illuminates appropriately
 */

double illuminatePoint(Vec3f &pt, Vec3f &cam, Vec3f &light,Wall &w, mesh &parent_mesh, int wallInd){
    double ambient = Ka * Ia;
    Vec3f n = w.n;
    Vec3f l = (light - pt).normalize();
    Vec3f v = (cam - pt).normalize();
    double dist = (cam - pt).length();
    double fatt = min(1.0,1/(c1 + c2 * dist + c3 *dist*dist));
    double diffusion = Kd * Ip * fatt * l.dot(n);
    if(diffusion < 0)
        diffusion = 0;
    //     cout << "BAMM!" << endl;
    double specular = Ks * Ip * fatt * pow(((2 * l.dot(n) * v.dot(n)) - l.dot(v)),specN);
    if(specular < 0)
    //     cout << "BAMM!!" << endl;
        specular = 0.0;
    int f = 1;
    
    if(ShadowCheckIntersection(light,pt,-1,parent_mesh)){
        f = 0;
    }

    double ans =  ambient + f * max(diffusion + specular,0.0);

    return ans;

}

/**
 * To illuminate a point on a wall, when multiple meshes are present
 * Finds if the light intersects any of the meshes on its way from source
 * to that point and illuminates appropriately
 */

double illuminatePoint(Vec3f &pt, Vec3f &cam, Vec3f &light,Wall &w, vector<mesh> &meshes, int wallInd){
    double ambient = Ka * Ia;
    Vec3f n = w.n;
    Vec3f l = (light - pt).normalize();
    Vec3f v = (cam - pt).normalize();
    double dist = (cam - pt).length();
    double fatt = min(1.0,1/(c1 + c2 * dist + c3 *dist*dist));
    double diffusion = Kd * Ip * fatt * l.dot(n);
    if(diffusion < 0)
        diffusion = 0;

    double specular = Ks * Ip * fatt * pow(((2 * l.dot(n) * v.dot(n)) - l.dot(v)),specN);

    if(specular < 0)
        specular = 0.0;
    
    int f = 1;

    for(int i = 0; i < meshes.size(); i++){
        if(ShadowCheckIntersection(light,pt,-1,meshes[i])){
            f = 0;
            break;
        }
    }

    double ans =  ambient + f * max(diffusion + specular,0.0);

    return ans;
}

/**
 * Finds intersection point of a ray with a plane
 */

Vec3f IntersectionPoint(Vec3f pix, Vec3f cam, Wall &w){
    double t1 = (pix - cam).dot(w.n);
    if(t1 == 0)
        return Vec3f(1e8,1e8,1e8);
    Vec3f arbpt = (w.a + w.b + w.c)/3.0;
    double t2 = (arbpt - cam).dot(w.n);
    double t = t2/t1;
    Vec3f pt = cam + (pix - cam)*t;
    return pt;
}

/**
 * Finds intersection point of a ray with a plane( a face in the mesh)
 */
Vec3f IntersectionPoint(Vec3f pix, Vec3f cam, int k, mesh &parent_mesh){
    double t1 = (pix - cam).dot(parent_mesh.faces[k].n);
    if(t1 == 0)
        return Vec3f(1e8,1e8,1e8);
    Vec3f arbpt = (parent_mesh.vertices[parent_mesh.faces[k].a] + parent_mesh.vertices[parent_mesh.faces[k].b] + parent_mesh.vertices[parent_mesh.faces[k].c])/3.0;
    double t2 = (arbpt - cam).dot(parent_mesh.faces[k].n);
    double t = t2/t1;
    Vec3f pt = cam + (pix - cam)*t;
    return pt;
}

void raycast(mesh &mymesh,Vec3f cam, Vec3f light, string &filename){
    bool visible_faces[mymesh.num_faces];
    init_bufs();
    
    backface_culling(mymesh, cam, visible_faces);

    for(int i = 0; i <= WALL_SIDE; i++){
        for(int j = 0; j <= WALL_SIDE; j++){
            Vec3f pix(i,j,0);
            Vec3f pt;
            bool isObject = false;
            for(int k = 0; k < mymesh.num_faces; k++){
                
                if(visible_faces[k]){
                    pt = IntersectionPoint(pix,cam,k,mymesh);
                    if(isOnFace(pt,mymesh.faces[k],mymesh) && zBuffer[i][j] < pt.z){
                        zBuffer[i][j] = pt.z;
                        isObject = true;
                        // cout << "Yo  " << k << endl;
                        double fR = mymesh.faces[k].col.R/255.0, fG = mymesh.faces[k].col.G/255.0, fB = mymesh.faces[k].col.B/255.0;
                        RGBtoHSV(fR,fG,fB,hsvraster[i][j][0],hsvraster[i][j][1],hsvraster[i][j][2]);
                        hsvraster[i][j][2] = illuminatePoint(pt,cam,light,k,mymesh);                        
                    }
                }
            }

            if(isObject)
                continue;
            
            for(int k = 0; k < 5; k++){                  
                pt = IntersectionPoint(pix,cam,walls[k]);
                if(isOnFace(pt,walls[k]) && zBuffer[i][j] < pt.z){
                    // cout << "WALL " << k << endl;
                    zBuffer[i][j] = pt.z;
                    double fR = walls[k].col.R/255.0, fG = walls[k].col.G/255.0, fB = walls[k].col.B/255.0;
                    RGBtoHSV(fR,fG,fB,hsvraster[i][j][0],hsvraster[i][j][1],hsvraster[i][j][2]);
                    hsvraster[i][j][2] = illuminatePoint(pt,cam,light,walls[k],mymesh,k);
                }
            }
            
        }
    }

    NormaliseV();
    make_ppm(filename);
}

void raycast(vector<mesh> &meshes,Vec3f cam, Vec3f light, string &filename){
    int no_faces = 0, offset = 0;
    for(int i = 0; i < meshes.size();i++)
        no_faces += meshes[i].num_faces;
    init_bufs();
    bool visible_faces[no_faces];
    for(int i = 0; i < meshes.size();i++){
        backface_culling(meshes[i],cam,visible_faces + offset);
        offset = offset + meshes[i].num_faces;
    }

    for(int i = 0; i <= WALL_SIDE; i++){
        for(int j = 0; j <= WALL_SIDE; j++){
            Vec3f pix(i,j,0);
            Vec3f pt;
            bool isObject = false;
            offset = 0;
            for(int k = 0; k < meshes.size(); k++){
                for(int l = 0; l < meshes[k].num_faces; l++){
                    if(visible_faces[offset + l]){
                        pt = IntersectionPoint(pix,cam,l,meshes[k]);
                        if(isOnFace(pt,meshes[k].faces[l],meshes[k]) && zBuffer[i][j] < pt.z){
                            zBuffer[i][j] = pt.z;
                            isObject = true;
                            double fR = meshes[k].faces[l].col.R/255.0, fG = meshes[k].faces[l].col.G/255.0, fB = meshes[k].faces[l].col.B/255.0;
                            RGBtoHSV(fR,fG,fB,hsvraster[i][j][0],hsvraster[i][j][1],hsvraster[i][j][2]);
                            hsvraster[i][j][2] = illuminatePoint(pt,cam,light,l,k,meshes);
                        }
                    }
                }
                offset += meshes[k].num_faces;
            }

            if(isObject)
                continue;

            for(int k = 0; k < 5; k++){                  
                pt = IntersectionPoint(pix,cam,walls[k]);
                if(isOnFace(pt,walls[k]) && zBuffer[i][j] < pt.z){
                    // cout << "WALL " << k << endl;
                    zBuffer[i][j] = pt.z;
                    double fR = walls[k].col.R/255.0, fG = walls[k].col.G/255.0, fB = walls[k].col.B/255.0;
                    RGBtoHSV(fR,fG,fB,hsvraster[i][j][0],hsvraster[i][j][1],hsvraster[i][j][2]);
                    hsvraster[i][j][2] = illuminatePoint(pt,cam,light,walls[k],meshes,k);
                }
            }
            
        }
    }
    NormaliseV();
    make_ppm(filename);
}


void make_ppm(string &filename){
    ofstream file;
	file.open(filename.c_str());
	file << "P3\n";
	file << WALL_SIDE+1 << " " << WALL_SIDE+1 << "\n";
	file << "255\n";
	// cout << "Hello" << endl;
	
	for(int i = WALL_SIDE; i >= 0; i--){

		for(int j = 0; j <= WALL_SIDE; j++){
			file << raster[j][i][0] << " " << raster[j][i][1] << " " << raster[j][i][2] << " ";
		}
		
		file << "\n";
	}	
	file << "\n";
	file.close();
}

int main(){
    string meshname("cube.obj");
    color col;
    col.R = 128; col.G = 10;col.B = 128;
    Vec3f cen(100, 100, -100);
    Vec3f cam(250, 250, 250);
    Vec3f light(100,100,-10);
    mesh mymesh(meshname,col,cen,70);
    vector<mesh> meshes;
    meshes.push_back(mymesh);
    meshname = "cube.obj";

    cen.x = 400; cen.y = 400;
    col.R = 0; col.G = 10;col.B = 128;
    mesh mymesh2(meshname,col,cen,70);
    meshes.push_back(mymesh2);

    cen.x = 100; cen.y = 400;
    col.R = 128; col.G = 10;col.B = 0;
    mesh mymesh3(meshname,col,cen,70);
    meshes.push_back(mymesh3);

    cen.x = 400; cen.y = 100;
    col.R = 128; col.G = 100;col.B = 255;
    mesh mymesh4(meshname,col,cen,70);
    meshes.push_back(mymesh4);

    string output("out.ppm");
    // raycast(mymesh2,cam,light,output);
    raycast(meshes,cam,light,output);

    
    // raycast(mymesh,cam,light,output);

    double x1,x2,x3;
    double thetaX = 0,thetaY = 0,thetaZ =0;
    double magnitude;
    double dtheta = 0.05;
    double theta = 0;

    vector<mesh> temp;

    for (int i = 0; i < 100; ++i)
	{
        char name[20];
		sprintf(name,"cube%03d.ppm",i+1);
		string cppname(name);
		cout << "Plotting " << cppname << endl;
        for(int j = 0; j < meshes.size(); j++){
		    temp.push_back(mesh(meshes[j],thetaX,thetaY,thetaZ));
        }
		raycast(temp,cam,light,cppname);
        temp.clear();
		theta += dtheta;
		x1 =  cos(theta);
		x2 =  sin(2*theta);
		x3 =  sin(3*theta);
		magnitude = sqrt(x1*x1 + x2*x2 + x3*x3);
		thetaX = 0;
		thetaY = acos(x1/magnitude);
		thetaZ = 0;
	}
}