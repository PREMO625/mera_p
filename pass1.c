#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MNT 50
#define MAX_MDT 200
#define MAX_ALA 50

struct MNT { int index; char name[32]; int mdt_index; };
struct MDT { int index; char label[32]; char opcode[32]; char operand[128]; };
struct ALA { int index; char arg[32]; };

void replaceArgsWithPosition(const char *operand, struct ALA ALA[], int ar, char *out) {
    // Replace occurrences of &ARGx with #pos
    out[0] = '\0';
    int i=0;
    while (operand[i]) {
        if (operand[i] == '&') {
            char token[64]; int t=0; int j=i;
            while (operand[j] && operand[j] != ' ' && operand[j] != ',' && operand[j] != '\n' && operand[j] != '\r') {
                token[t++]=operand[j++]; 
            }
            token[t]='\0';
            int found = 0;
            for (int k=1;k<ar;k++){
                if (strcmp(ALA[k].arg, token)==0) { char num[8]; sprintf(num,"#%d",k); strcat(out,num); found=1; break; }
            }
            if (!found) strcat(out, token); // keep as-is
            i = j;
        } else {
            strncat(out, &operand[i], 1);
            i++;
        }
    }
}

int main() {
    FILE *src = fopen("source.txt","r");
    if (!src) { perror("source.txt"); return 1; }

    struct MNT mnt[MAX_MNT];
    struct MDT mdt[MAX_MDT];
    struct ALA ala[MAX_ALA];
    char line[256];
    int mntc=0, mdtc=0, arcurr=1;

    while (fgets(line, sizeof(line), src)) {
        char label[64]="", opcode[64]="", operand[128]="";
        // parse label, opcode and rest as operand (operand may contain spaces)
        int fields = sscanf(line, "%s %s %[^\n]", label, opcode, operand);
        if (fields < 2) continue; // blank/malformed line -> skip
        if (strcmp(opcode,"END")==0) break;

        if (strcmp(opcode,"MACRO")==0) {
            // read macro header (next non-empty line)
            while (fgets(line, sizeof(line), src)) {
                if (sscanf(line, "%s %s %[^\n]", label, opcode, operand) >= 2) break;
            }
            // add to MNT
            strcpy(mnt[mntc].name, opcode);
            mnt[mntc].index = mntc;
            mnt[mntc].mdt_index = mdtc;
            mntc++;

            // parse formal args (operand might be like "&ARG1,&ARG2")
            arcurr = 1;
            char argscopy[128];
            strcpy(argscopy, operand);
            char *tk = strtok(argscopy, ",");
            while (tk) {
                // trim spaces
                while (*tk==' ') tk++;
                strcpy(ala[arcurr].arg, tk);
                ala[arcurr].index = arcurr;
                arcurr++;
                tk = strtok(NULL, ",");
            }

            // read macro body until MEND
            while (fgets(line, sizeof(line), src)) {
                char l2[64]="", op2[64]="", opd2[128]="";
                if (sscanf(line, "%s %s %[^\n]", l2, op2, opd2) < 2) continue;
                if (strcmp(op2,"MEND")==0) {
                    // add MEND entry
                    mdt[mdtc].index = mdtc;
                    strcpy(mdt[mdtc].label, "-");
                    strcpy(mdt[mdtc].opcode, "MEND");
                    strcpy(mdt[mdtc].operand, "-");
                    mdtc++;
                    break;
                }
                // operand may contain formal param -> replace &ARG with positional #n
                char replaced[256]; replaced[0]='\0';
                replaceArgsWithPosition(opd2, ala, arcurr, replaced);
                mdt[mdtc].index = mdtc;
                strcpy(mdt[mdtc].label, l2);
                strcpy(mdt[mdtc].opcode, op2);
                strcpy(mdt[mdtc].operand, replaced);
                mdtc++;
            }
        }
    }

    fclose(src);

    // print & write tables
    printf("\n=== MDT ===\nIdx\tLabel\tOpcode\tOperand\n");
    for (int i=0;i<mdtc;i++) printf("%d\t%s\t%s\t%s\n",mdt[i].index,mdt[i].label,mdt[i].opcode,mdt[i].operand);
    printf("\n=== MNT ===\nIdx\tName\tMDT_index\n");
    for (int i=0;i<mntc;i++) printf("%d\t%s\t%d\n",mnt[i].index,mnt[i].name,mnt[i].mdt_index);
    printf("\n=== ALA ===\nIdx\tArg\n");
    for (int i=1;i<arcurr;i++) printf("%d\t%s\n",ala[i].index,ala[i].arg);

    FILE *f;
    f = fopen("MNT.txt","w"); for (int i=0;i<mntc;i++) fprintf(f,"%d %s %d\n",mnt[i].index,mnt[i].name,mnt[i].mdt_index); fclose(f);
    f = fopen("MDT.txt","w"); for (int i=0;i<mdtc;i++) fprintf(f,"%d %s %s %s\n",mdt[i].index,mdt[i].label,mdt[i].opcode,mdt[i].operand); fclose(f);
    f = fopen("ALA.txt","w"); for (int i=1;i<arcurr;i++) fprintf(f,"%d %s\n",ala[i].index,ala[i].arg); fclose(f);

    printf("\nPass 1 completed successfully!\n");
    return 0;
}
