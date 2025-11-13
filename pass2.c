#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MNT 50
#define MAX_MDT 200

struct MNT { char name[32]; int mdt_index; };
struct MDT { char label[32]; char opcode[32]; char operand[128]; };

void substitute_and_print(FILE *out, struct MDT *m, char actualArgs[][64], int argCount) {
    char temp[256]; temp[0]='\0';
    int i=0;
    while (m->operand[i]) {
        if (m->operand[i] == '#') {
            i++;
            int num = 0;
            while (m->operand[i] >= '0' && m->operand[i] <= '9') {
                num = num*10 + (m->operand[i]-'0'); i++;
            }
            if (num>=1 && num<=argCount) strcat(temp, actualArgs[num-1]);
        } else if (m->operand[i] == '&') {
            char token[64]; int t=0; int j=i;
            while (m->operand[j] && m->operand[j] != ' ' && m->operand[j] != ',' ) token[t++]=m->operand[j++]; token[t]='\0';
            int pick = 0;
            for (int k=0;k<argCount;k++) {
                if (strstr(token, actualArgs[k]) != NULL) { pick = k+1; break; }
            }
            if (pick) strcat(temp, actualArgs[pick-1]);
            i = j;
        } else {
            strncat(temp, &m->operand[i], 1);
            i++;
        }
    }
    fprintf(out, "%s\t%s\t%s\n", m->label, m->opcode, temp);
    printf("%s\t%s\t%s\n", m->label, m->opcode, temp);
}

int main() {
    FILE *f = fopen("MNT.txt","r");
    if (!f) { perror("MNT.txt"); return 1; }
    struct MNT mnt[MAX_MNT];
    int mntc=0;
    while (fscanf(f, "%d %31s %d", &mntc, mnt[mntc].name, &mnt[mntc].mdt_index) == 3) {
        break;
    }
    rewind(f);
    mntc=0;
    while (fscanf(f, "%d %31s %d", &mntc, mnt[mntc].name, &mnt[mntc].mdt_index) == 3) mntc++;
    fclose(f);
    f = fopen("MDT.txt","r");
    if (!f) { perror("MDT.txt"); return 1; }
    struct MDT mdt[MAX_MDT];
    int mdtc=0;
    while (fscanf(f, "%d %31s %31s %127[^\n]", &mdtc, mdt[mdtc].label, mdt[mdtc].opcode, mdt[mdtc].operand) == 4) mdtc++;
    fclose(f);

    
    FILE *src = fopen("source.txt","r");
    FILE *out = fopen("expanded_output.txt","w");
    if (!src || !out) { perror("source or out"); return 1; }

    char line[256];
    printf("\n=== Expanded Assembly Code ===\n");
    printf("Label\tOpcode\tOperand\n");
    fprintf(out, "Label\tOpcode\tOperand\n");

    while (fgets(line, sizeof(line), src)) {
        char label[64]="", opcode[64]="", operand[128]="";
        if (sscanf(line, "%s %s %[^\n]", label, opcode, operand) < 2) continue;
        if (strcmp(opcode,"MACRO")==0) {
            
            while (fgets(line, sizeof(line), src)) {
                char l2[64], o2[64], opd2[128];
                if (sscanf(line, "%s %s %[^\n]", l2, o2, opd2) >= 2 && strcmp(o2,"MEND")==0) break;
            }
            continue;
        }
        if (strcmp(opcode,"END")==0) {
            fprintf(out,"%s\t%s\t%s\n", label, opcode, operand);
            printf("%s\t%s\t%s\n", label, opcode, operand);
            break;
        }
        
        int found = -1;
        for (int i=0;i<mntc;i++) if (strcmp(opcode, mnt[i].name)==0) { found=i; break; }

        if (found >= 0) {
           
            char actualArgs[20][64]; int argc=0;
            char copy[128]; strcpy(copy, operand);
            char *tk = strtok(copy, ",");
            while (tk) { 
                while (*tk==' ') tk++;
                strcpy(actualArgs[argc++], tk);
                tk = strtok(NULL, ",");
            }
            
            int idx = mnt[found].mdt_index;
            for (int j = idx; j < mdtc && strcmp(mdt[j].opcode,"MEND")!=0; j++) {
                substitute_and_print(out, &mdt[j], actualArgs, argc);
            }
        } else {
            fprintf(out,"%s\t%s\t%s\n", label, opcode, operand);
            printf("%s\t%s\t%s\n", label, opcode, operand);
        }
    }

    fclose(src);
    fclose(out);
    printf("\nExpanded code written to expanded_output.txt\n");
    return 0;
}
