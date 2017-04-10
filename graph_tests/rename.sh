
for f in $(ls *.rb)
do
  IFS='.' read -r -a array <<< "$f"
  echo $array
  mv $f "${array}_spec.rb"
done
