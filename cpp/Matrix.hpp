/*
*	Implementing Matrix and Operations 
*	for Cpp Practice
*/
#pragma once

#include <iostream>
#include <vector>

// Matrix class
// Basic matrix implementation, Ensure operation only for numeric types
// example:
// Matrix<type> objName(row, col);
// or
// Matrix<type> objName{ { 1, 2, ... }, ... };
template <typename T>
class Matrix
{
	std::vector<std::vector<T>> data;
	size_t rows;
	size_t cols;

	// Row-by Class for [] operator
	class Proxy 
	{
		std::vector<T>& rowRef;
	public:
		Proxy(std::vector<T>& r) : rowRef(r) {}
		T& operator[](size_t j) { return rowRef[j]; }
		const T& operator[](size_t j) const { return rowRef[j]; }
	};

public:
	// Constructor
	Matrix(size_t row, size_t col) :
		rows(row),
		cols(col),
		data(row, std::vector<T>(col)) {
	}
	Matrix(std::initializer_list<std::initializer_list<T>> init) :
		rows(init.size()),
		cols(init.begin()->size()),
		data{} 
	{
		data.reserve(rows);
		for (auto& rowList : init) data.emplace_back(rowList);
	}

	// Getter
	size_t row() const { return rows; }
	size_t col() const { return cols; }

	// () operator access
	T& operator()(size_t i, size_t j) { return data[i][j]; }
	T operator()(size_t i, size_t j) const { return data[i][j]; }

	// [] operator access
	Proxy operator[](size_t i) { return Proxy(data[i]); }
	const Proxy operator[](size_t i) const { return Proxy(const_cast<std::vector<T>&>(data[i])); }

	// Scalar Addition
	Matrix operator+(const T& scalar) const 
	{
		Matrix result(rows, cols);
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < cols; ++j)
			{
				result(i, j) = data[i][j] + scalar;
			}
		}
		return result;
	}

	// Self Scalar Addition
	void operator+=(const T& scalar)
	{
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < cols; ++j)
			{
				data[i][j] += scalar;
			}
		}
	}

	// Matrix Addition
	Matrix operator+(const Matrix& other) const
	{
		if (rows != other.rows || cols != other.cols) { throw std::invalid_argument("Invalid matrix dimensions for addition"); }
		Matrix result(rows, cols);
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < cols; ++j)
			{
				result(i, j) = data[i][j] + other(i, j);
			}
		}
		return result;
	}

	// Scalar Multiplication
	Matrix operator*(const T& scalar) const
	{
		Matrix result(rows, cols);
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < cols; ++j)
			{
				result(i, j) = data[i][j] * scalar;
			}
		}
		return result;
	}

	// Self Scalar Multiplication
	void operator*=(const T& scalar)
	{
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < cols; ++j)
			{
				data[i][j] *= scalar;
			}
		}
	}

	// Matrix Multiplication
	Matrix operator*(const Matrix& other) const
	{
		if (cols != other.rows) { throw std::invalid_argument("Invalid matrix dimensions for multiplication"); }
		Matrix result(rows, other.cols);
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < other.cols; ++j)
			{
				for (size_t k = 0; k < cols; ++k)
				{
					result(i, j) += data[i][k] * other(k, j);
				}
			}
		}
		return result;
	}

	// Transpose
	Matrix transpose() const
	{
		Matrix result(cols, rows);
		for (size_t i = 0; i < rows; ++i)
		{
			for (size_t j = 0; j < cols; ++j)
			{
				result(j, i) = data[i][j];
			}
		}
		return result;
	}

	// Self Transpose
	void transpose_inplace()
	{
		Matrix temp = this->transpose();
		*this = std::move(temp);
	}

	// Inverse
	Matrix<double> inverse() const 
	{
		if (rows != cols) { throw std::invalid_argument("Inverse is defined only for square matrices"); }

		const size_t n = rows;
		Matrix<double> temp = *this;

		Matrix<double> result(n, n);  // unit matrix
		for (size_t i = 0; i < n; ++i) { result(i, i) = static_cast<double>(1); }

		// Gauss-Jordan Elimination
		for (size_t i = 0; i < n; ++i) {
			// select pivot
			double pivot = static_cast<double>(temp(i, i));
			if (pivot == static_cast<double>(0.0))
			{
				size_t swapRow = i + 1;
				while (swapRow < n && temp(swapRow, i) == static_cast<double>(0.0)) { ++swapRow; }
				if (swapRow == n) { throw std::runtime_error("Matrix is singular, cannot invert"); }
				std::swap(temp.data[i], temp.data[swapRow]);
				std::swap(result.data[i], result.data[swapRow]);
				pivot = temp(i, i);
			}

			// normalize pivot
			for (size_t j = 0; j < n; ++j) 
			{
				temp(i, j) /= pivot;
				result(i, j) /= pivot;
			}

			// remove row
			for (size_t r = 0; r < n; ++r) 
			{
				if (r == i) continue;
				T factor = temp(r, i);
				for (size_t c = 0; c < n; ++c) 
				{
					temp(r, c) -= factor * temp(i, c);
					result(r, c) -= factor * result(i, c);
				}
			}
		}
		return result;
	}

	// Inverse type retention
	Matrix inverse_retention() const
	{
		if (rows != cols) { throw std::invalid_argument("Inverse is defined only for square matrices"); }

		const size_t n = rows;
		Matrix<T> temp = *this;

		Matrix<T> result(n, n);  // unit matrix
		for (size_t i = 0; i < n; ++i) { result(i, i) = static_cast<T>(1); }

		// Gauss-Jordan Elimination
		for (size_t i = 0; i < n; ++i) {
			// select pivot
			T pivot = temp(i, i);
			if (pivot == static_cast<T>(0))
			{
				size_t swapRow = i + 1;
				while (swapRow < n && temp(swapRow, i) == static_cast<T>(0)) { ++swapRow; }
				if (swapRow == n) { throw std::runtime_error("Matrix is singular, cannot invert"); }
				std::swap(temp.data[i], temp.data[swapRow]);
				std::swap(result.data[i], result.data[swapRow]);
				pivot = temp(i, i);
			}

			// normalize pivot
			for (size_t j = 0; j < n; ++j)
			{
				temp(i, j) /= pivot;
				result(i, j) /= pivot;
			}

			// remove row
			for (size_t r = 0; r < n; ++r)
			{
				if (r == i) continue;
				T factor = temp(r, i);
				for (size_t c = 0; c < n; ++c)
				{
					temp(r, c) -= factor * temp(i, c);
					result(r, c) -= factor * result(i, c);
				}
			}
		}
		return result;
	}

	// Self Inverse
	void inverse_inplace()
	{
		Matrix temp = this->inverse_retention();
		*this = std::move(temp);
	}

	// Determinant
	double determinant() const
	{
		if (rows != cols) { throw std::invalid_argument("Determinant is defined only for square matrices"); }

		Matrix<double> temp = *this;
		double det = (double)1;

		// Gauss-Jordan Elimination / LU decomposition
		for (size_t i = 0; i < rows; ++i)
		{
			// pivot check
			if (temp(i, i) == static_cast<double>(0))
			{
				size_t swapRow = i + 1;
				while (swapRow < rows && temp(swapRow, i) == static_cast<double>(0)) { ++swapRow; }
				if (swapRow == rows) { return 0; }
				std::swap(temp.data[i], temp.data[swapRow]);
				det = -det;
			}

			det *= temp(i, i);
			if (temp(i, i) == static_cast<double>(0)) { return 0; }

			// remove row
			for (size_t j = i + 1; j < rows; ++j)
			{
				T factor = temp(j, i) / temp(i, i);
				for (size_t k = i; k < cols; ++k)
				{
					temp(j, k) -= factor * temp(i, k);
				}
			}
		}
		return det;
	}

	// Determinant type retention
	T determinant_retention() const
	{
		if (rows != cols) { throw std::invalid_argument("Determinant is defined only for square matrices"); }

		Matrix<T> temp = *this;
		T det = (T)1;

		// Gauss-Jordan Elimination / LU decomposition
		for (size_t i = 0; i < rows; ++i)
		{
			// pivot check
			if (temp(i, i) == static_cast<T>(0))
			{
				size_t swapRow = i + 1;
				while (swapRow < rows && temp(swapRow, i) == static_cast<T>(0)) { ++swapRow; }
				if (swapRow == rows) { return 0; }
				std::swap(temp.data[i], temp.data[swapRow]);
				det = -det;
			}

			det *= temp(i, i);
			if (temp(i, i) == static_cast<T>(0)) { return 0; }

			// remove row
			for (size_t j = i + 1; j < rows; ++j)
			{
				T factor = temp(j, i) / temp(i, i);
				for (size_t k = i; k < cols; ++k)
				{
					temp(j, k) -= factor * temp(i, k);
				}
			}
		}
		return det;
	}

	// debug print
	void print() const
	{
		for (const auto& r : data)
		{
			for (const auto& e : r) std::cout << e << ' ';
			std::cout << '\n';
		}
	}
};
