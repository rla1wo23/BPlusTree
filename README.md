# BPlusTree

1. B+tree 자료구조 구현(in cpp)
   -2023/Spring Database과제로 작성한 B+Tree입니다.
   -binary file을 읽어서 -대표적으로 Node는 Vector를 이용해 구현하였음. -포인터 저장소 또한 Vector를 이용해 구현함.
   -Node는, leaf Node와 Non-leaf의 기능이 전혀 다르지만, (key 값은 둘 다 저장하지만, leaf는 value를, Non-leaf는 포인터를 저장함) 같은 structure를 이용함. 이는 split과정을 재귀적으로 수행하는 데에 용이하게 하기 위함이다.
   -insert 함수를, split 발생여부를 기준으로 8개의 케이스로 분기하여 branch를 만듬. (split 발생, non-leaf split 발생, 연쇄 split 발생 이렇게 3가지가 있고, 없고에 따라, 8개의 케이스로 분기함) ->구현의 대부분의 시간을 이 케이스와 오류 해결에 소모함.
2. binary 파일에 write하기 구현
   1. header creating: 그냥 입력값을, 첫번째 4바이트에 기록하게끔 함.
   2. insertion:
      2-1:input 파일을 구분자 |를 기준으로 split하여 insertion을 수행
      2-2:그 다음 이것을 바탕으로 B+트리에 삽입함
      2-3:leaf노드부터 insertion 함, 각 노드 별 모든 정보를 읽어와서 binary 파일에 write함.
