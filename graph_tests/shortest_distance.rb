require 'redis'

redis = Redis.new

describe 'shortest path' do
  before do
    redis.flushdb
    redis.gvertex 'graph1', 'a', 'b', 'c'
    redis.gedge 'graph1', 'a', 'b', 1
    redis.gedge 'graph1', 'a', 'b', 3
    redis.gedge 'graph1', 'b', 'c', 2

    @result = redis.gshortestpath('graph1', 'a', 'c')
    @result2 = redis.gshortestpath('graph1', 'a', 'c')
  end

  it 'should make the path from a to c = 5' do
    @result.last.should eq "5"
  end

  it 'should have the right order of the path' do
    @result[0].should eq 'a'
    @result[1].should eq 'b'
    @result[2].should eq 'c'
  end

  it 'should print the correct vertices' do
    vertices = redis.gvertices('graph1')
    vertices.should eq ['a', 'b', 'c']
  end

end
