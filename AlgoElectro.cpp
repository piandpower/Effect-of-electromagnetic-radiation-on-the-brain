#include "Materials.h"
#include "GridCreator.h"
#include <fstream>
#include <cstring>
#include "ElectromagneticSource.h"
#include "Node3DField.h"







void communicate(GridCreator mesh){
/*à faire*/

}

/* convention:Table array contient le maillage du MPI+ les voisins*/
/* convention: nbrElts ne contient pas les voisins*/


double Compute_dt(GridCreator mesh){
    double dx=mesh.deltaX;
    double dy=mesh.deltaY;
    double dz=mesh.deltaZ;
    double dt=0.0;
    double tmp=0.0;
    double c=0.0;
    int i=0;                                                                                
    for (i=0;i<mesh.materials.numberOfMaterials;i++){
            double mu_material=mesh.materials.getProperty(Materials_object[i].T_in,i,4);         /* !!!!!!!!!!!!!!/*� faire avec T initial*/     
            double epsilon_material=mesh.materials.getProperty(Materials_object[i].T_in,i,5);    /* !!!!!!!!!!!!!!/*� faire avec T initial*/
            c=1/(sqrt(mu_material*epsilon_material));
            if(i==0){
                dt=1/(c*sqrt(1/(dx*dx) + 1/(dy*dy) + 1/(dz*dz)));
            }
            else{
                tmp=1/(c*sqrt(1/(dx*dx) + 1/(dy*dy) + 1/(dz*dz)));
                if(tmp<dt){
                    dt=tmp;
                }
            }
    }
    return dt;
}

void update(GridCreator mesh,double dt,double t_current){   
/* update magnetic Field */
        int i,j,k;
        for(k=1;k<mesh.numberOfNodesInEachDir[0];k++){
            for(j=1;j<mesh.numberOfNodesInEachDir[1];j++){
                for(i=1;i<mesh.numberOfNodesInEachDir[2];i++){
                    
                    double T=mesh.nodesMagn(i,j,k).Temperature;
                    double mu_material=mesh.materials.getProperty(T,4,mesh.nodesMagn(i,j,k).material); 
                    
                    double Transfo[3];                                                     /* transformer les indices locaux en globaux voir Fonction Chris et Alex*/ 
                    Transfo=Transformation(i,j,k,mesh.myrank,mesh.numberofprocess) ;      /* mettre dans gridcreator "numberofProcess" et " myrank"  */                         
                    if(isInsideSource(Transfo[0],Transfo[1],Transfo[2])){                
                        /* Fonction à faire  dans Electromagnetic Source composantes donne l'info si c'est E ou H et x ou y ou z */
                        mesh.nodesMagn(i,j,k).field[0]=mesh.input_parser.source.getvalue(i,j,k,t_current,composants_1);
                        mesh.nodesMagn(i,j,k).field[1]=mesh.input_parser.source.getvalue(i,j,k,t_current,composants_2);
                        mesh.nodesMagn(i,j,k).field[2]=mesh.input_parser.source.getvalue(i,j,k,t_current,composants_3);
                    }
                    else{

                        /* update magnetic field H_x */
                        mesh.nodesMagn(i,j,k).field[0]= mesh.nodesMagn(i,j,k).field[0] + (dt/(mu_material*mesh.deltaZ))*(mesh.nodesElec(i,j,k).field[1]-mesh.nodesElec(i,j,k-1).field[1]) - (dt/(mu_material*mesh.deltaY))*(mesh.nodesElec(i,j,k).field[2]-mesh.nodesElec(i,j-1,k).field[2]);
                        /* update magnetic Field H_y */
                        mesh.nodesMagn(i,j,k).field[1]= mesh.nodesMagn(i,j,k).field[1]  + (dt/(mu_material*mesh.deltaX))*(mesh.nodesElec(i,j,k).field[2]-mesh.nodesElec(i-1,j,k).field[2]) -  (dt/(mu_material*mesh.deltaZ))*(mesh.nodesElec(i,j,k).field[0]-mesh.nodesElec(i,j,k-1).field[0]);
                        /* update magnetic Field H_z */
                        mesh.nodesMagn(i,j,k).field[2]= mesh.nodesMagn(i,j,k).field[2]  + (dt/(mu_material*mesh.deltaY))*(mesh.nodesElec(i,j,k).field[0]-mesh.nodesElec(i,j-1,k).field[0]) -  (dt/(mu_material*mesh.deltaX))*(mesh.nodesElec(i,j,k).field[1]-mesh.nodesElec(i-1,j,k).field[1]);

                        }
                    }
                }
            }
        /* update electric field  */
          for(k=1;k<mesh.numberOfNodesInEachDir[0];k++){
            for(j=1;j<mesh.numberOfNodesInEachDir[1];j++){
                for(i=1;i<mesh.numberOfNodesInEachDir[2];i++){
                    
                    double T=mesh.nodesElec(i,j,k).Temperature;
                    double epsilon_material=mesh.materials.getProperty(T,4,mesh.nodesElec(i,j,k).material);
                    if(isInsideSource(Transfo[0],Transfo[1],Transfo[2])){
                        mesh.nodesMagn(i,j,k).field[0]=mesh.input_parser.source.getvalue(i,j,k,t_current,composants_4);
                        mesh.nodesMagn(i,j,k).field[1]=mesh.input_parser.source.getvalue(i,j,k,t_current,composants_5);
                        mesh.nodesMagn(i,j,k).field[2]=mesh.input_parser.source.getvalue(i,j,k,t_current,composants_6);
                    }else{
                    /* update magnetic field E_x */
                    mesh.nodesElec(i,j,k).field[0]= mesh.nodesElec(i,j,k).field[0] + (dt/(epsilon_material*mesh.deltaY))*(mesh.nodesMagn(i,j+1,k).field[2]-mesh.nodesMagn(i,j,k).field[2]) - (dt/(epsilon_material*mesh.deltaZ))*(mesh.nodesMagn(i,j,k+1).field[1]-mesh.nodesMagn(i,j,k).field[1]);
                    /* update magnetic Field E_y */
                    mesh.nodesElec(i,j,k).field[1]= mesh.nodesElec(i,j,k).field[1] + (dt/(epsilon_material*mesh.deltaZ))*(mesh.nodesMagn(i,j,k+1).field[0]-mesh.nodesMagn(i,j,k).field[0])  - (dt/(epsilon_material*mesh.deltaX))*(mesh.nodesMagn(i+1,j,k).field[2]-mesh.nodesMagn(i,j,k).field[2]);
                    /* update magnetic Field E_z */
                    mesh.nodesElec(i,j,k).field[2]= mesh.nodesElec(i,j,k).field[2] + (dt/(epsilon_material*mesh.deltaX))*(mesh.nodesMagn(i+1,j,k).field[1]-mesh.nodesMagn(i,j,k).field[1]) - (dt/(epsilon_material*mesh.deltaY))*(mesh.nodesMagn(i,j+1,k).field[0]-mesh.nodesMagn(i,j,k).field[0]);
                         }
                        }
                    }
                }
           
}


/*Fonction principale*/
void run(GridCreator mesh){
    double t_current=0.0;
    double t_final=mesh.input_parser.tempsfinal;                                      /*rajouter le temps final dans le fichier*/
    /*calcul condition CFL*/
    double dt=Compute_dt(mesh);
    /*loop over time*/
    while(t_current<t_final){        
        void communicate(mesh);                              /* à faire*/
        void update(mesh,dt,t_current);
        t_current=t_current+dt;
    }
}




