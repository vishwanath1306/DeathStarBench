container_list=( $(docker ps -aq) )
for c in ${container_list[@]}; do
	echo $c
	docker exec ${c} tar CcF $(dirname /dev/shm) - $(basename /dev/shm) | sudo tar CxF . - 
done

