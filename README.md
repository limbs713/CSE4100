# CSE4100  Project
서강대학교 "멀티코어 프로그래밍" 과목에서 실습한 프로젝트입니다.


## Project 1
### 프로젝트 설명
간단한 shell의 다양한 기능을 구현해보는 프로젝트입니다
#### phase 1
cd, rmdir, mkdir, ls, cat, echo 같은 간단한 리눅스 명령어를 구현한다.

#### phase 2
pipe(|) 명령어를 구현하여 명령어의 출력을 연결하여 다른 명령어의 입력으로 사용할 수 있게 한다.

#### phase 3
background process의 구현과 process의 상태 전환등을 구현하였다. 

### 과제 코드 점수
phase1 : 100/100

phase2 : 100/100

phase3 : 100/100

-------
## Project 2
### 프로젝트 설명
multi-thread와 socket network의 개념을 실습하기 위한 프로젝트로 주식 시장처럼 실시간으로 발생하는 매매와 매수를 반영하여 주식을 관리하는 기능을 구현.

동시에 다양한 thread에서 데이터에 접근하기 때문에 semaphore로 데이터 일관성을 유지함.

### 과제 코드 점수

Project : 100/100

------
## Project 3
### 프로젝트 설명
malloc의 기능을 구현하는 프로젝트로 system call을 이용해 heap size를 저장할 데이터의 크기에 맞게 증가 혹은 감소시키는 기능을 구현.

free list 를 linked list로 관리하는 explicit free list 의 형태로 구현하였다.

best-fit 방식을 통해 할당할 메모리 탐색 시 최적의 free block을 탐색하여 반환하도록 구현하였다.

### 과제 코드 점수

Project : 91/100
