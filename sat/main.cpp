#include <iostream>
#include <QFile>
#include <QTextStream>
#include <QStack>
#include <string>
#include <cstring>
#include <stack>
#include <ostream>
#include "NodeBoolTree.h"
#include "boolinterval.h"
#include "boolequation.h"
#include "BBV.h"
#include "../Allocator.cpp"


int main(int argc, char *argv[])
{
    static AllocatorPool<BoolInterval, 1000> boolIntervalAlloc;
    static AllocatorPool<BoolEquation, 1000> boolEquationAlloc;
    static AllocatorPool<NodeBoolTree, 200> nodeBoolTreeAlloc;
    static AllocatorPool<BBV, 50> bbvAlloc;

    QStringList full_file_list;
    QList<QStringList> Elements;
    std::string filepath;
    QStringList inputs;
    //std::cout << "Input file path...\n";
    //std::cin >> filepath;
    // Hardcode input
    //	filepath = "sat_ex_2.pla";
    filepath = "Sat_ex11_3.pla";
    // filepath = "Sat_ex30_3.pla";
    QFile file(QString::fromUtf8(filepath.c_str()));

    //считываем весь файл
    if ((file.exists()) && (file.open(QIODevice::ReadOnly))) {
        while (!file.atEnd()) {
            full_file_list << file.readLine().replace("\r\n", "");
        }

        int cnfSize = full_file_list.length();
        BoolInterval **CNF = new BoolInterval*[cnfSize];
        int rangInterval = -1; // error

        if (cnfSize) {
            rangInterval = full_file_list[0].toUtf8().trimmed().length();
        }

        for (int i = 0; i < cnfSize; i++) { // Заполняем массив
            QString strv = full_file_list[i];
            CNF[i] = new (boolIntervalAlloc.Allocate(sizeof(BoolInterval))) BoolInterval(strv.toUtf8().trimmed().data());
        }

        QString rootvec = "";
        QString rootdnc = "";

        //Строим интервал в которм все компоненты принимают значение '-',
        //который представляет собой корень уравнения, пока пустой.
        //В процессе поиска корня, компоненты интервала буду заменены на конкретные значения.

        for (int i = 0; i < rangInterval; i++) {
            rootvec += "0";
            rootdnc += "1";
        }

        QByteArray v = rootvec.toUtf8();
        BBV* vec = new (bbvAlloc.Allocate(sizeof(BBV))) BBV(v.data());
        QByteArray d = rootdnc.toUtf8();
        BBV* dnc = new (bbvAlloc.Allocate(sizeof(BBV))) BBV(d.data());

        // Создаем пустой корень уравнения;
        BoolInterval *root = new (boolIntervalAlloc.Allocate(sizeof(BoolInterval))) BoolInterval(*vec, *dnc);

        BoolEquation *boolequation = new (boolEquationAlloc.Allocate(sizeof(BoolEquation))) 
            BoolEquation(CNF, root, cnfSize, cnfSize, *vec);

        bool rootIsFinded = false;
        stack<NodeBoolTree *> BoolTree;
        NodeBoolTree *startNode = new (nodeBoolTreeAlloc.Allocate(sizeof(NodeBoolTree))) NodeBoolTree(boolequation);
        BoolTree.push(startNode);

        do {
            NodeBoolTree *currentNode(BoolTree.top());

            if (currentNode->lt == nullptr &&
                    currentNode->rt == nullptr) { // Если вернулись в обработанный узел
                BoolEquation *currentEquation = currentNode->eq;
                bool flag = true;

                // Цикл для упрощения по правилам.
                while (flag) {
                    int a = currentEquation->CheckRules(); // Проверка выполнения правил

                    switch (a) {
                        case 0: { // Корня нет.
                            BoolTree.pop();
                            flag = false;
                            break;
                        }

                        case 1: { // Правило выполнилось, корень найден или продолжаем упрощать.
                            if (currentEquation->count == 0 ||
                                    currentEquation->mask.getWeight() ==
                                    currentEquation->mask.getSize()) { // Если кончились строки или столбцы, корень найден.
                                flag = false;
                                rootIsFinded =
                                    true; // Полагаем, что корень найден, выполняем проверку корня

                                for (int i = 0; i < cnfSize; i++) {
                                    if (!CNF[i]->isEqualComponent(*currentEquation->root)) {
                                        rootIsFinded = false;//Корень не найден. Продолжаем искать дальше.
                                        BoolTree.pop();
                                        break;
                                    }
                                }
                            }
                            break;
                        }

                        case 2: { // Правила не выполнились, ветвление.
                            // Ветвление, создание новых узлов.
                            int indexBranching = currentEquation->ChooseColForBranching();

                            BoolEquation *Equation0 = new (boolEquationAlloc.Allocate(sizeof(BoolEquation))) 
                                BoolEquation(*currentEquation);
                            BoolEquation *Equation1 = new (boolEquationAlloc.Allocate(sizeof(BoolEquation))) 
                                BoolEquation(*currentEquation);

                            Equation0->Simplify(indexBranching, '0');
                            Equation1->Simplify(indexBranching, '1');

                            NodeBoolTree *Node0 = new (nodeBoolTreeAlloc.Allocate(sizeof(NodeBoolTree))) 
                                NodeBoolTree(Equation0);
                            NodeBoolTree *Node1 = new (nodeBoolTreeAlloc.Allocate(sizeof(NodeBoolTree))) 
                                NodeBoolTree(Equation1);

                            currentNode->lt = Node0;
                            currentNode->rt = Node1;

                            BoolTree.push(Node1);
                            BoolTree.push(Node0);

                            flag = false;
                            break;
                        }
                    }
                }
            } else {
                BoolTree.pop();
            }

        } while (BoolTree.size() > 1 && !rootIsFinded);

        if (rootIsFinded) {
            cout << "Root is:\n ";
            BoolInterval *finded_root = BoolTree.top()->eq->root;
            cout << string(*finded_root) << endl;
        } else {
            cout << "Root is not exists!";
        }

        // Clean up allocated memory
        for (int i = 0; i < cnfSize; i++) {
            CNF[i]->~BoolInterval();
            boolIntervalAlloc.Deallocate(CNF[i]);
        }
        delete[] CNF;
        
        vec->~BBV();
        bbvAlloc.Deallocate(vec);
        dnc->~BBV();
        bbvAlloc.Deallocate(dnc);
        
        root->~BoolInterval();
        boolIntervalAlloc.Deallocate(root);
        
        boolequation->~BoolEquation();
        boolEquationAlloc.Deallocate(boolequation);
        
        // The NodeBoolTree objects will be cleaned up when the stack goes out of scope

    } else {
        std::cout << "File does not exists.\n";
    }

    return 0;
}
