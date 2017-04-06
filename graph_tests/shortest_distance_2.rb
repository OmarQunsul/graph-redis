require 'redis'

redis = Redis.new

describe 'shortest path' do
  before do
    redis.flushdb
    redis.gvertex 'graph1', *(%w(a b c d e f g))
    redis.gedge 'graph1', 'a', 'c', 1
    redis.gedge 'graph1', 'a', 'b', 2
    redis.gedge 'graph1', 'b', 'c', 3
    redis.gedge 'graph1', 'e', 'c', 5
    redis.gedge 'graph1', 'd', 'c', 3
    redis.gedge 'graph1', 'd', 'f', 1
    redis.gedge 'graph1', 'e', 'f', 4
    redis.gedge 'graph1', 'g', 'f', 9
    redis.gedge 'graph1', 'a', 'g', 15

    @result = redis.gshortestpath('graph1', 'a', 'g')
  end

  it 'should make the path from a to g = 14' do
    @result.last.should eq "14"
  end

  it 'should have the right order of the path' do
    @result[0].should eq 'a'
    @result[1].should eq 'c'
    @result[2].should eq 'd'
    @result[3].should eq 'f'
    @result[4].should eq 'g'
  end

  it 'should print the correct vertices' do
    vertices = redis.gvertices('graph1', '*')
    vertices.should eq %w(a b c d e f g)
  end
end
